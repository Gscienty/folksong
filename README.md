# Folksong I/O代理服务

Folksong是一个基于libuv实现的模块化I/O处理服务，是一种以Sidecar模式构建的服务网关。
Folksong允许向应用无侵入地添加多种I/O功能，避免了为了使用第三方组件而向应用添加额外代码，使应用代码更专注于业务逻辑的实现。

本项目现阶段主要针对Kafka的消息代理，在编译时需要依赖多个第三方类库。使用包括`libuv`中的I/O事件驱动框架，使用`librdkafka`实现对Kafka消息的收发操作，
使用`nodejs/http-parser`实现对HTTP Request的解析，使用`libpcre`实现正则表达式的相关功能。

## Folksong 依赖第三方类库

* `libuv`
* `librdkafka`
* `nodejs/http-parser`
* `libpcre`

## Kafka 消费者代理

Folksong支持对Kafka消费者角色的消息消费代理。Folksong将代理监听指定的Kafka topic消息，并将消息转化为标准输入，通过管道输送给指定的可执行文件。
若使用该功能模块，则应该在conf中写如下配置项：

```
kafka {

    # 指定Kafka broker
    config bootstrap.servers 127.0.0.1:9092;

    # 指定Kafka Consumer所在的group id
    config group.id temp-group;

    # 指定Kafka监听的topic
    topic listen_topic1 {
        # 消费Kafka消息时触发执行的脚本，消息的value部分将以stdin流入
        # 支持两个变量：
        # $kafka_topic: 为消费消息的topic
        # $kafka_key: 为消费消息的key
        exec /bin/python3 /home/y/x/x/script.py $kafka_topic $kafka_key;
    }
}
```

## Timer 时钟触发器

该模块主要对libuv中`uv_timer_t`进行封装，可在此基础上实现心跳机制，该模块主要目标是对接Prometheus的监控定时推送任务。配置样例如下：

```
timer {
    # 当Folksong启动后的多久后触发时钟触发器
    timeout 1s;

    # 轮询多少时间后触发时钟触发器
    repeat 15s;

    # 时钟触发器触发执行的脚本
    echo /bin/sh -c "echo 'knock knock.'";
}
```

### kafka 生产者代理

该模块主要实现Kafka生产者角色的消息发送过程的代理。Folksong对Kafka的发送过程进行封装，业务代码将以HTTP协议的方式发送给Folksong，
再由Folksong转换为Kafka消息投递到Kafka队列中。
```
http {
    # Folksong对内暴露的IP和接口
    host 127.0.0.1;
    port 10010;

    # http模块本身支持Route
    # 设定Kafka推送接口 HTTP Request Path
    # 可以在Path的正则表达式中设定named substring
    # 其中topic为指定推送的topic，key为指定推送的key，优先级最高
    kafka '^/publish(/(?<topic>.*?)(/(?<key>.*))?)?$' {

        # 指定Kafka broker
        config bootstrap.servers 127.0.0.1:9092;
        # 指定Kafka Producer所在的group id
        config group.id temp-group;

        # 指定Request Header Parameters中哪个参数的值为topic，优先级次于Path中指定的topic
        topic_param 'Kafka-Topic';
        # 指定Request Header Parameters中哪个参数的值为key, 优先级仅次于Path中指定的key
        key_param 'Kafka-Key';

        # 指定默认topic，优先级最低
        default_topic 'test_topic';
        # 指定默认key，优先级最低
        default_key 'test_key';
    }
}
```
