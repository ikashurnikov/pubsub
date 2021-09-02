PubSub = Publish/Subscribe

'exchanges.AGRO.COWS.books'
'exchanges.AGRO.COWS.anndeal'
'exchanges.AGRO.EGGS.anndeal'

class Data
{
};

using DataPtr = std::shared_ptr<Data>;

1. Single process under linux. Multithreaded.
2. Simple, robust (fool proof).
3. Publishing performance is the key.
4. Subscribe/unsubscribe performance - irrelevant.
5. If subscribed - ALL updates must be delivered. Skipping is not allowed.
6. Quantities:
   Subscribers - up to 3000.
   Publishers  - ~20.
  99.9999% of all updates to a SINGLE channel is done by SAME publisher from same thread.
  Average number of subscribers to every channel is ~5.
   Number of channels - ~200 000.
   Total number of publishes per second - 50000
7. As a designer of a system - wee have a right to state some requirements on clients. But only some.
8. Channels are created when first accessed. Subscription can come before first publish.
   No need to delete channels.
10. Rare data case: when subscribing - we publish to new subscriber ONE last published data in this channel.

