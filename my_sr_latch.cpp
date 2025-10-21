#include <iostream>
#include <chrono>
#include <thread>

using namespace std;

// Симуляция времени в наносекундах
using Time = uint64_t;

class Clock 
{
public:
    Time current_time_ns = 0;
    Time period_ns;

    Clock(Time period_ns_) : period_ns(period_ns_) {}

    // Продвинуть время на 1 период и сгенерировать фронт
    void tick() 
    {
        current_time_ns += period_ns;
        cout << "[Clock] Tick: time = " << current_time_ns << " ns\n";
    }
};

class SRTLatch 
{
private:
    bool q_ = false;
    bool nq_ = true;

    // Входы
    bool s_ = false;
    bool r_ = false;

    // Время последних изменений входов
    Time last_s_change_ = 0;
    Time last_r_change_ = 0;

    // Временные параметры (в наносекундах)
    Time setup_time_ns_;
    Time hold_time_ns_;

    // Последнее время фронта CLK
    Time last_clock_edge_ = 0;

public:
    SRTLatch(Time setup_time_ns, Time hold_time_ns)
        : setup_time_ns_(setup_time_ns), hold_time_ns_(hold_time_ns) {}

    // Установка входов с учётом времени
    void set_inputs(bool s, bool r, Time current_time) 
    {
        if (s != s_) 
        {
            s_ = s;
            last_s_change_ = current_time;
            cout << "[Input] S changed to " << s_ << " at " << current_time << " ns\n";
        }
        if (r != r_) 
        {
            r_ = r;
            last_r_change_ = current_time;
            cout << "[Input] R changed to " << r_ << " at " << current_time << " ns\n";
        }
    }

    // Обработка фронта тактового сигнала
    void clock_edge(Time current_time) 
    {
        cout << "[Clock edge] at " << current_time << " ns\n";

        // Проверка setup time для S и R:
        // Входы должны быть стабильны за setup_time до фронта
        if ((current_time - last_s_change_) < setup_time_ns_) 
        {
            cout << "!!! Setup time violation on S input! (stable for " << (current_time - last_s_change_) << " ns, required " << setup_time_ns_ << " ns)\n";
        }
        if ((current_time - last_r_change_) < setup_time_ns_) 
        {
            cout << "!!! Setup time violation on R input! (stable for " << (current_time - last_r_change_) << " ns, required " << setup_time_ns_ << " ns)\n";
        }

        // Проверка hold time:
        // Предположим, что входы не изменяются сразу после фронта (для моделирования можно проверить позже)
        // Здесь покажем пример: просто сохраним время фронта для будущих проверок
        last_clock_edge_ = current_time;

        // В реальном симуляторе hold time проверяют, что входы не изменились в интервале после фронта

        // Обновляем выходы (логика SR latch)
        if (s_ && r_) 
        {
            cout << "!!! Invalid state: S = 1 and R = 1\n";
            // Обычно состояние неопределено, в коде зададим q = false
            q_ = false;
            nq_ = false;
        } else if (s_) 
        {
            q_ = true;
            nq_ = false;
        } else if (r_) 
        {
            q_ = false;
            nq_ = true;
        } // если S=0 и R=0 — сохраняется предыдущее состояние

        print_state();
    }

    // Для демонстрации: проверка hold time (нужно вызывать при изменении входов)
    void check_hold(Time current_time) 
    {
        if ((current_time - last_clock_edge_) < hold_time_ns_) {
            cout << "!!! Hold time violation: inputs changed " << (current_time - last_clock_edge_) << " ns after clock edge, hold time is " << hold_time_ns_ << " ns\n";
        }
    }

    void print_state() const 
    {
        cout << "Q = " << q_ << ", /Q = " << nq_ << "\n";
    }
};

int main() 
{
    // Параметры: период тактового сигнала 100 ns, setup_time = 20 ns, hold_time = 10 ns
    Clock clk(100);
    SRTLatch latch(20, 10);

    // Симуляция событий:
    // Время 0 ns: устанавливаем входы
    latch.set_inputs(false, false, clk.current_time_ns);

    // Имитация изменений и фронтов

    // Шаг 1: в 50 ns меняем S = 1
    clk.current_time_ns = 50;
    latch.set_inputs(true, false, clk.current_time_ns);

    // Шаг 2: в 80 ns меняем R = 0 (остается 0)
    clk.current_time_ns = 80;
    latch.set_inputs(true, false, clk.current_time_ns);

    // Шаг 3: фронт CLK в 100 ns
    clk.current_time_ns = 100;
    latch.clock_edge(clk.current_time_ns);

    // Шаг 4: меняем S = 0 слишком рано — 105 ns (нарушение hold time)
    clk.current_time_ns = 105;
    latch.set_inputs(false, false, clk.current_time_ns);
    latch.check_hold(clk.current_time_ns);

    // Шаг 5: фронт CLK в 200 ns
    clk.current_time_ns = 200;
    latch.clock_edge(clk.current_time_ns);

    // Шаг 6: меняем R = 1 слишком поздно — 195 ns (нарушение setup time)
    clk.current_time_ns = 195;
    latch.set_inputs(false, true, clk.current_time_ns);

    // Шаг 7: фронт CLK в 200 ns (снова)
    clk.current_time_ns = 200;
    latch.clock_edge(clk.current_time_ns);

    return 0;
}

