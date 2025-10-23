#include <tgbot/tgbot.h>

using namespace std;
using namespace TgBot;

int main() {
    // 🔑 Вставь сюда токен от BotFather
    string token = "????";

    Bot bot(token);

    // Ответ на команду /start
    bot.getEvents().onCommand("start", [&bot](const Message::Ptr& message) {
        bot.getApi().sendMessage(message->chat->id, "Hi! How are you?");
    });

    // Ответ на любое текстовое сообщение
    bot.getEvents().onAnyMessage([&bot](const Message::Ptr& message) {
        if (StringTools::startsWith(message->text, "/")) return; // не повторять команды
        bot.getApi().sendMessage(message->chat->id, "If you're Rityonochek, I love you! " + message->text);
    });

    try {
        printf("Бот запущен. Ожидание сообщений...\n");
        TgLongPoll longPoll(bot);
        while (true) {
            longPoll.start();
        }
    } catch (const exception& e) {
        printf("Ошибка: %s\n", e.what());
    }

    return 0;
}
