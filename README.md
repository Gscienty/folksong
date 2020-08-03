# Folksong

Folksong是一个基于libuv实现的模块化I/O处理框架，是一种Sidecar模式下的服务网关。
Folksong允许向应用无侵入地添加多种功能，避免了为了使用第三方组件而向应用添加额外代码，使应用代码更专注于业务逻辑的实现。

## Kafka 消费者代理

Folksong支持对Kafka消费者角色的消息消费代理。Folksong将代理监听指定的Kafka topic消息，并将消息转化为标准输入，通过管道输送给指定的可执行文件。
若使用该功能模块，则应该在conf中写如下配置项：

```
kafka_listener {

    # 指定Kafka broker
    kafka_config bootstrap.servers 127.0.0.1:9092;

    # 指定Kafka Consumer所在的group id
    kafka_config group.id temp-group;

    # 指定Kafka监听的topic
    kafka_topic listen_topic1 listen_topic2 listen_topic3;

    # 消费Kafka消息时触发执行的脚本，消息的value部分将以stdin流入
    # 支持两个变量：
    # $kafka_topic: 为消费消息的topic
    # $kafka_key: 为消费消息的key
    exec /bin/python3 /home/y/x/x/script.py $kafka_topic $kafka_key;

}
```
