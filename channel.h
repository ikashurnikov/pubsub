#pragma once

#include "data.h"
#include "func_traits.h"
#include <thread>
#include <vector>
#include <string_view>
#include <iostream>
#include <mutex>
#include <assert.h>

namespace internal {

//----------------------------------------------------------------------
// Subscription
//----------------------------------------------------------------------

class Subscription {
public:
    explicit Subscription(void* key) : key_(key){} // Ключ можно поменять, сделал для простоты

    virtual ~Subscription() = default;

    const void* GetKey() const { return key_; }

    virtual void Call(const DataPtr& data) = 0;

    template<class Func>
    static std::shared_ptr<Subscription> Make(void* key, Func&& func)
    {
        return std::make_shared<SubscriptionImpl<Func>>(key, std::forward<Func>(func));
    }

private:
    void* key_;
};

template<class Func>
class SubscriptionImpl : public Subscription {
public:
    using FuncTraits = FunctionTraits<Func>;
    using ArgType = typename  FuncTraits::template Arg<0> ::Type;  // ->const  std::shared_ptr<T>&    
    using ShPtrType = std::remove_reference_t<std::remove_cv_t<ArgType>>; // -> std::shared_ptr<T>
    using DataType = typename ShPtrType::element_type; // T

    SubscriptionImpl(void* key, Func func)
        :Subscription(key),
         func_(std::move(func)) {}

    void Call(const std::shared_ptr<::Data>& data) override {
        if (auto d = std::dynamic_pointer_cast<DataType>(data)) {
            func_(d);
        }
        else {
            // TODO: Это нормальная ситуация ?
        }
    }

private:
    Func func_;
};

using SubscriptionPtr = std::shared_ptr<Subscription>;

//----------------------------------------------------------------------
// Channel
//----------------------------------------------------------------------

class Channel {
public:
    using SubscriptionsList = std::vector<SubscriptionPtr>;
    using SubscriptionsListPtr = std::shared_ptr<SubscriptionsList>;

    Channel(){
        subscriptions_ = std::make_shared<SubscriptionsList>();
    }

    void Subscribe(SubscriptionPtr subscription){
        SubscriptionsList subs = GetSubscribtions();
        subs.push_back(subscription);
        SetSubscribtions(std::move(subs));
    }

    void Unsubscribe(void* key){
        SubscriptionsList subs = GetSubscribtions();
        auto it = std::remove_if(subs.begin(), subs.end(), [key](const auto& s) {return s->GetKey() == key; });
        if (it != subs.end()) {
            subs.erase(it, subs.end());
            SetSubscribtions(std::move(subs));
        }
    }

    void SendData(const DataPtr& data) {
        SubscriptionsListPtr sub_list; {
            std::unique_lock<std::mutex> lock(mutex_);
            sub_list = subscriptions_;
            last_data_ = data;
        }

        for (const auto& sub : *sub_list) {
            try {
                sub->Call(data);
            }
            catch (const std::exception& exc) {
                std::cerr << "Unexpected exception: " << exc.what() << std::endl;
                assert(false);
            }
        }
    }

    void SendLastData(const std::shared_ptr<internal::Subscription>& sub){
        if (auto data = lastData()) {
            sub->Call(data);
        }
    }

    SubscriptionsList GetSubscribtions() const{
        std::unique_lock<std::mutex> lock(mutex_);
        return *subscriptions_;
    }

    void SetSubscribtions(SubscriptionsList&& list){
        auto new_list = std::make_shared<SubscriptionsList>(std::move(list));
        std::unique_lock<std::mutex> lock(mutex_);
        subscriptions_ = std::move(new_list);
    }

    DataPtr lastData() const{
        std::unique_lock<std::mutex> lock(mutex_);
        return last_data_;
    }

private:
    mutable std::mutex mutex_;
    SubscriptionsListPtr subscriptions_;
    DataPtr last_data_;
};


//----------------------------------------------------------------------
// ChannelList
//----------------------------------------------------------------------

class ChannelList{
public:
    explicit ChannelList(std::vector<std::string>&& channel_names)
    {
        channel_names_ = std::move(channel_names);
        channels_map_.reserve(channel_names_.size());
        for (const std::string& name : channel_names_) {
            channels_map_.emplace(name, std::make_unique<Channel>());
        }
    }

    ChannelList(ChannelList&&) = default;

    template<class Func>
    void Subscribe(std::string_view channel_name, void* key, Func&& func){
        auto it = channels_map_.find(channel_name);
        if (it == channels_map_.end())
        {
            std::cerr << "Failed to subscribe to the channel. Channel \"" << channel_name << "\" not found" << std::endl;
            assert(false);
            return;
        }

        auto& channel = it->second;
        auto new_subscription = internal::Subscription::Make(key, std::forward<Func>(func));

        std::unique_lock<std::mutex> lock(mutex_);
        channel->Subscribe(new_subscription);
        lock.unlock();

        // Rare data case: when subscribing - we publish to new subscriber ONE last published data in this channel.
        channel->SendLastData(new_subscription);
    }

    void Unsubscribe(void* key){
        // Subscribe/unsubscribe performance - irrelevant.
        std::unique_lock<std::mutex> lock(mutex_);
        for (const auto& p : channels_map_) {
            auto& channel = p.second;
            channel->Unsubscribe(key);
        }
    }

    Channel* GetChannel(std::string_view name) const{
        struct Cache{
            std::string name;
            Channel* channel = nullptr;
        };
        
        /* 99.9999% of all updates to a SINGLE channel is done by SAME publisher from same thread. */
        thread_local static Cache hot_path;
        if (hot_path.name == name) {
            return hot_path.channel;
        }

        auto it = channels_map_.find(name);
        if (it != channels_map_.end()) {
            hot_path.name = name;
            hot_path.channel = it->second.get();
            return it->second.get();
        }
        return nullptr;
    }

private:
    std::mutex mutex_;
    std::vector<std::string> channel_names_;
    std::unordered_map<std::string_view, std::unique_ptr<Channel>> channels_map_;
};

}