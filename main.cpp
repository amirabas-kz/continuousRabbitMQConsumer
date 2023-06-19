#include <iostream>
#include <amqpcpp.h>
#include <amqpcpp/libev.h>
#include <ev.h>
#include <chrono>
#include <thread>

class MyLibEvHandler : public AMQP::LibEvHandler
{
public:
    MyLibEvHandler(struct ev_loop *loop) : AMQP::LibEvHandler(loop) {}
};

class MyHandler
{
public:
    MyHandler()
    {
        // Create a new connection when the previous one fails
        connection = nullptr;

        loop = ev_loop_new(EVFLAG_AUTO);
        handler = new MyLibEvHandler(loop);
    }

    ~MyHandler()
    {
        delete handler;
        ev_loop_destroy(loop);
    }

    void run()
    {
        connection = new AMQP::TcpConnection(handler, AMQP::Address("amqp://username:password@IP/vhost"));

        // Perform your operations on the connection
        if (connection->usable())
        {
            std::cout << "Connected to RabbitMQ" << std::endl;

            // Create a channel
            AMQP::TcpChannel channel(connection);

            channel.onError([&](const char *message)
                            {
                std::cout << "Channel error: " << message << std::endl;
                // Close the connection and set the flag to reconnect
                connection->close();
                reconnect(); });

            // Consume messages from the queue
            channel.consume("routing_queue", AMQP::noack).onReceived([&](const AMQP::Message &message, uint64_t deliveryTag, bool redelivered)
                                                                     {
                                                                         std::cout << "Received message: " << message.body() << std::endl;
                                                                         // Process the received message

                                                                     });

            ev_run(loop, 0);
        }
        else
        {
            std::cout << "Failed to connect to RabbitMQ" << std::endl;
            delete connection;
            reconnect();
        }
    }

private:
    void reconnect()
    {
        std::cout << "Connection closed. Reconnecting..." << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(1));
        run();
    }

    MyLibEvHandler *handler;
    AMQP::TcpConnection *connection;
    struct ev_loop *loop;
};

int main()
{
    MyHandler handler;
    handler.run();

    return 0;
}
