#include "runtime.h"
#include <chrono>
#include <sstream>
#include <csignal>

class BooksData : public Data{};
class AnndealData : public Data{};

//----------------------------------------------------------------------
// DataSource
//----------------------------------------------------------------------

class DataSource {
public:
    virtual ~DataSource() = default;
    
    virtual void Stop() = 0;

    template<class T>
    static std::unique_ptr<DataSource> Make(const std::string& channel_name) {
        return std::make_unique<DataSourceImpl<T>>(channel_name);
    }
};

template<class DataType>
class DataSourceImpl :  public DataSource{
public:
    DataSourceImpl(std::string channel_name)
        :channel_name_(std::move(channel_name))
    {
        running_ = true;
        thread_ = std::thread(&DataSourceImpl::ThreadProc, this);
    }

    void Stop(){
        running_ = false;
        thread_.join();
    }

private:
    void ThreadProc(){
        constexpr size_t call_count = 50000; //�Total number of publishes per second - 50000

        while (running_) {
            const auto now = std::chrono::high_resolution_clock::now();
            for (size_t i = 0; i < call_count; ++i) {
                runtime()->Publish(channel_name_, std::make_shared<DataType>());
            }
            const auto end = std::chrono::high_resolution_clock::now();

            const auto time = std::chrono::duration_cast<std::chrono::milliseconds>(end - now).count();

            std::stringstream ss;
            ss << "Channel " << channel_name_ << "Call count = " << call_count << ". Time " << time << "ms\n";
            std::cout << ss.str();
        }
    }
   
private:
    std::string channel_name_;
    std::thread thread_;
    std::atomic<bool> running_;
};

//----------------------------------------------------------------------
// DataSink
//----------------------------------------------------------------------

class DataSink{
public:
    virtual ~DataSink() = default;

    void IncCallCount() {
        count_++;
    }

    template<class T>
    static std::unique_ptr<DataSink> Make(const std::string& channel_name) {
        return std::make_unique<DataSinkImpl<T>>(channel_name);
    }

    virtual void Stop() = 0;

    size_t GetCallCount() const { return count_; }

private:
    std::atomic<size_t> count_ = { 0 };
};

template<class DataType>
class DataSinkImpl : public DataSink {
public:
    DataSinkImpl(const std::string& channel_name) 
        : channel_name_(channel_name)
    {
        runtime()->Subscribe(this, channel_name, 
            [this](const std::shared_ptr<DataType>& data) {
                IncCallCount();
            }
        );
    }

    void Stop() override {
        runtime()->Unsubsribe(this);
        std::cout << "Sink " << channel_name_ << ". Data count: " << GetCallCount() << std::endl;
    }

private:
    std::string channel_name_;
};

//----------------------------------------------------------------------
// SubscribeUnsubcsribe
//----------------------------------------------------------------------

class SubscribeUnsubcsribe {
public:
    SubscribeUnsubcsribe() {
        thread_ = std::thread(&SubscribeUnsubcsribe::TheadProc, this);
    }

    void Stop(){
        runing_ = false;
        thread_.join();
    }

private:
    void TheadProc() {
        while (runing_){
            runtime()->Subscribe(this, "exchanges.AGRO.COWS.books",[](const DataPtr&) {});
            runtime()->Subscribe(this, "exchanges.AGRO.EGGS.anndeal", [](const DataPtr&) {});

            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            runtime()->Unsubsribe(this);
        }
    }

private:
    std::atomic<bool> runing_ = { true };
    std::thread thread_;
};

//----------------------------------------------------------------------
// main
//----------------------------------------------------------------------


int main(int argc, char** argv)
{
    static std::atomic<bool> running{ true };

    auto sig_handler = [](int) {
        running = false;
    };

    signal(SIGINT, sig_handler);
 #ifdef SIGBREAK
    signal(SIGBREAK, sig_handler); // handles Ctrl-Break on Win32
 #endif
    signal(SIGABRT, sig_handler);
    signal(SIGTERM, sig_handler);



    ChannelsBuilder channels;
    channels
        .Channel("exchanges.AGRO.COWS.books")
        .Channel("exchanges.AGRO.COWS.anndeal")
        .Channel("exchanges.AGRO.EGGS.anndeal");

    Runtime runtime(channels.Build());
    Runtime::SetRuntime(&runtime);

    std::vector<std::unique_ptr<DataSource>> sources;
    sources.push_back(DataSource::Make<BooksData>("exchanges.AGRO.COWS.books"));
    sources.push_back(DataSource::Make<AnndealData>("exchanges.AGRO.COWS.anndeal"));

    std::vector<std::unique_ptr<DataSink>> sinks;
    for (size_t i = 0; i < 5; ++i) {
        sinks.push_back(DataSink::Make<BooksData>("exchanges.AGRO.COWS.books"));
        sinks.push_back(DataSink::Make<AnndealData>("exchanges.AGRO.COWS.anndeal"));
    }
   
    SubscribeUnsubcsribe test_subunsub;

    std::cout << "Press CTRL-C to exit" << std::endl;
    while (running) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    
    for (auto& src : sources) {
        src->Stop();
    }

    for (auto& sink : sinks) {
        sink->Stop();
    }

    test_subunsub.Stop();

    return 0;
}
