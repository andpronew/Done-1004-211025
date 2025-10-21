/* Поиск кратчайшего пути между городами по алгоритму Дейкстры
Хранить расстояния от стартовой вершины до всех остальных (вектор dist).
Использовать приоритетную очередь для выбора вершины с минимальным расстоянием.
Восстанавливать кратчайший путь через массив предков (parent). */

#include <iostream>      
#include <string>       
#include <unordered_map> // Для таблицы city_to_index_
#include <vector>        
#include <queue>         // Для priority_queue в алгоритме Дейкстры
#include <stack>         // Для восстановления пути
#include <limits>        // Для numeric_limits<int>::max()

using namespace std;

// Структура узла списка смежности с весом ребра
struct WeightedNode {
    int vertex;         // Номер вершины (индекс города)
    int weight;         // Вес ребра (расстояние)
    WeightedNode* next; // Указатель на следующий узел в списке

    WeightedNode(int v, int w) : vertex(v), weight(w), next(nullptr) {} // Конструктор
};

class Graph {
private:
    int num_vertices_;           // Количество вершин (городов)
    WeightedNode** adj_lists_;   // Массив списков смежности (по одному списку на город)
    string* index_to_city_;      // Массив для отображения индекса в название города
    unordered_map<string, int> city_to_index_; // Отображение названия города в индекс

public:
    // Конструктор графа: принимает массив городов и их количество
    Graph(string cities[], int count) : num_vertices_(count) {
        adj_lists_ = new WeightedNode*[num_vertices_];   // Выделяем память под массив списков
        index_to_city_ = new string[num_vertices_];      // Память для имен городов

        for (int i = 0; i < num_vertices_; ++i) {
            adj_lists_[i] = nullptr;         // Изначально списки пустые
            index_to_city_[i] = cities[i];  // Запоминаем имя города по индексу
            city_to_index_[cities[i]] = i;  // И индекс по имени города
        }
    }

    // Метод для добавления ребра с весом (расстоянием)
    void add_edge(const string& from, const string& to, int distance) {
        int src = city_to_index_[from]; // Индекс города-источника
        int dest = city_to_index_[to];  // Индекс города-приёмника
        WeightedNode* new_node = new WeightedNode(dest, distance); // Создаём новый узел списка
        new_node->next = adj_lists_[src]; // Вставляем в начало списка смежности
        adj_lists_[src] = new_node;
    }

    // Метод для печати графа (списки смежности с расстояниями)
    void print_graph() const {
        for (int i = 0; i < num_vertices_; ++i) {
            cout << index_to_city_[i] << ": ";
            WeightedNode* temp = adj_lists_[i];
            if (!temp) {
                cout << "(no outgoing connections)"; // Если нет дорог из этого города
            }
            while (temp) {
                cout << index_to_city_[temp->vertex] << " (" << temp->weight << " km)"; // Печатаем город и расстояние
                temp = temp->next;
                if (temp) cout << " -> "; // Разделитель между соседями
            }
            cout << endl;
        }
    }

    // Алгоритм Дейкстры для поиска кратчайшего пути по расстоянию
    void dijkstra(const string& from, const string& to) {
        // Проверяем, что оба города есть в графе
        if (city_to_index_.count(from) == 0 || city_to_index_.count(to) == 0) {
            cout << "One of the cities is not in the network." << endl;
            return;
        }

        int start = city_to_index_[from]; // Индекс начального города
        int end = city_to_index_[to];     // Индекс конечного города

        vector<int> dist(num_vertices_, numeric_limits<int>::max()); // Вектор расстояний, инициализирован максимальным значением
        vector<int> parent(num_vertices_, -1);                       // Для восстановления пути: кто предок вершины
        vector<bool> visited(num_vertices_, false);                  // Отметки посещения вершин

        // Приоритетная очередь для выбора вершины с минимальным расстоянием
        // Пары: (текущая длина пути, индекс вершины)
        priority_queue<pair<int,int>, vector<pair<int,int>>, greater<pair<int,int>>> pq;

        dist[start] = 0;  // Расстояние до начальной вершины = 0
        pq.push({0, start}); // Добавляем начальную вершину в очередь

        // Основной цикл алгоритма
        while (!pq.empty()) {
            int current_dist = pq.top().first; // Текущая минимальная дистанция в очереди
            int current = pq.top().second;     // Вершина с этим минимальным расстоянием
            pq.pop();

            if (visited[current]) continue;    // Если вершина уже обработана, пропускаем
            visited[current] = true;           // Отмечаем как посещённую

            if (current == end) break;         // Если достигли цели — можно выйти

            WeightedNode* temp = adj_lists_[current]; // Проходим по всем соседям текущей вершины
            while (temp) {
                int neighbor = temp->vertex;  // Индекс соседней вершины
                int weight = temp->weight;    // Вес ребра (расстояние)

                // Если нашли более короткий путь к соседу — обновляем данные
                if (!visited[neighbor] && dist[current] + weight < dist[neighbor]) {
                    dist[neighbor] = dist[current] + weight; // Обновляем расстояние
                    parent[neighbor] = current;               // Запоминаем предка для восстановления пути
                    pq.push({dist[neighbor], neighbor});      // Добавляем в очередь
                }
                temp = temp->next; // Переходим к следующему соседу
            }
        }

        // Если расстояние до цели осталось максимальным — путь не найден
        if (dist[end] == numeric_limits<int>::max()) {
            cout << "No path from " << from << " to " << to << "." << endl;
            return;
        }

        // Восстанавливаем путь из массива parent, начиная с конца
        stack<int> path;
        for (int v = end; v != -1; v = parent[v])
            path.push(v);

        // Выводим путь и расстояние
        cout << "Shortest path from " << from << " to " << to << " (distance: " << dist[end] << " km): ";
        while (!path.empty()) {
            cout << index_to_city_[path.top()];
            path.pop();
            if (!path.empty()) cout << " -> ";
        }
        cout << endl;
    }

    // Деструктор — освобождаем память всех узлов списков
    ~Graph() {
        for (int i = 0; i < num_vertices_; ++i) {
            WeightedNode* current = adj_lists_[i];
            while (current) {
                WeightedNode* temp = current;
                current = current->next;
                delete temp;
            }
        }
        delete[] adj_lists_;
        delete[] index_to_city_;
    }
};

int main() {
    string cities[] = {"New York", "Los Angeles", "Kyiv", "Odessa"};
    int city_count = sizeof(cities) / sizeof(cities[0]);

    Graph city_graph(cities, city_count);

    // Добавляем дороги с расстояниями в км
    city_graph.add_edge("New York", "Los Angeles", 3944);
    city_graph.add_edge("New York", "Kyiv", 7533);
     city_graph.add_edge("New York", "Odessa", 7818);
      city_graph.add_edge("Los Angeles", "Kyiv", 10157);
	city_graph.add_edge("Los Angeles", "New York", 3944); 
	city_graph.add_edge("Los Angeles", "Odessa", 10562);
	 city_graph.add_edge("Kyiv", "Odessa", 652);
	 city_graph.add_edge("Kyiv", "Los Angeles", 10157);
	 city_graph.add_edge("Kyiv", "New York", 7533);
	city_graph.add_edge("Odessa", "Kyiv", 652);
         city_graph.add_edge("Odessa", "Los Angeles", 10562);
         city_graph.add_edge("Odessa", "New York", 7818);



    city_graph.print_graph(); // Выводим граф

    cout << endl;
    city_graph.dijkstra("New York", "Odessa");   
    city_graph.dijkstra("New York", "Kyiv");    
    city_graph.dijkstra("Kyiv", "Odessa");
city_graph.dijkstra("New York", "Los Angeles");     
    city_graph.dijkstra("Los Angeles", "Kyiv");      
    city_graph.dijkstra("Los Angeles", "Odessa");
    return 0;
}

