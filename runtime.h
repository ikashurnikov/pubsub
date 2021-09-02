#pragma once

#include "data.h"
#include "channel.h"

//----------------------------------------------------------------------
// ChannelsBuilder
//----------------------------------------------------------------------

class ChannelsBuilder {
public:
    ChannelsBuilder& Channel(std::string name) {
        names_.push_back(std::move(name));
        return *this;
    }

    std::unique_ptr<internal::ChannelList> Build() {
        return std::make_unique<internal::ChannelList>(std::move(names_));
    }

private:
    std::vector<std::string> names_;
};

//----------------------------------------------------------------------
// Runtime
//----------------------------------------------------------------------

class Runtime {
public:
    Runtime(std::unique_ptr<internal::ChannelList>&& channels)
        :channels_(std::move(channels)){}

    static void SetRuntime(Runtime* runtime) {
        g_rutimne = runtime;
    }

    static Runtime* GlobalRuntime() {
        return g_rutimne;
    }

    void Publish(std::string_view channel_name, const DataPtr& data) {
        if (auto channel = channels_->GetChannel(channel_name)) {
            channel->SendData(data);
        }
        else {
            std::cerr << "Failed to publish data. Channel \"" << channel_name << "\" not found" << std::endl;
        }
    }

    template<class Func>
    void Subscribe(void* key, std::string_view channel_name, Func&& func) {
        channels_->Subscribe(channel_name, key, std::forward<Func>(func));
    }

    void Unsubsribe(void* key){
        channels_->Unsubscribe(key);
    }

private:
    std::unique_ptr<internal::ChannelList> channels_;
    static Runtime* g_rutimne;
};

inline Runtime* runtime(){
    return Runtime::GlobalRuntime();
}

