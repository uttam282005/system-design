Realtime Pubsub

In Message Broker, whenever a publisher pushes a message to the message broker, it remains in the broker until the consumer pulls it from it. How message is pulled from the broker? All message brokers like AWS SQS provides APIs (or SDK) to do this.

In Pubsub, as soon as the publisher pushes a message to the Pubsub Broker, it immediately gets delivered to the consumers who are subscribed to this broker. Here, consumers don’t do an API call or anything to get the message. Message automatically gets delivered to the consumer in realtime by the Pubsub broker.

In short, in the message broker, consumers pull the message from the broker, while the Pubsub broker pushes the message to the consumer.
Press enter or click to view image in full size

One thing to note here is messages are not stored/retained in the Pubsub broker. As soon as the Pubsub broker receives the message, it pushes it to all the consumers who are subscribed to this channel and gets done with it. It does not store anything.

Example of Realtime Pubsub Broker: Redis

Redis is not only used for caching but also for real-time Pubsub as well.
Where to use Realtime Pubsub?

There can be a wide variety of use cases. When you want to build a very low latency application, you utilise this Pubsub feature.

One use case is when you want to build a real-time chatting application. For chatting applications, we use Websocket. But in a horizontally scaled environment, there can be many servers connected to different clients, as you can see in the below picture.
Press enter or click to view image in full size

If client-1 wants to send a message to client-3, he can’t send it directly because client-3 is not connected to server-1, so server-1 will not be able to deliver the message to client-3 after receiving the message from client-1.
You need to somehow deliver the message of client-1 to server-2 then server-2 can send this message to client-3. You can do this via Redis Pubsub. See the below picture.
