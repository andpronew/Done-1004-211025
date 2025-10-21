/* new Node(dest) — создаёт в heap новый узел.
delete temp — освобождает память, иначе будет утечка.
Граф хранится в виде массива списков смежности, каждый список построен вручную через указатели.
Это ориентированный граф — ребро от A к B не означает ребро от B к A, т.е. ребра - векторы 
На примере сети из пяти городов
*/

#include <iostream>

using namespace std;

// Структура одного узла в списке смежности: город на карте
struct City {
    int point;          // Номер вершины, на которую указывает ребро: номер города
    City* next;          // Указатель на следующий узел списка

    // Конструктор узла
    City(int v) : point(v), next(nullptr) {}
};

// Класс графа
class Graph {
private:
    int num_vertices_;     // Количество вершин в графе
    City** adj_lists_;     // Массив указателей на списки смежности

public:
    // Конструктор графа
    Graph(int vertices) : num_vertices_(vertices) {
        // Выделяем память под массив указателей на списки
        adj_lists_ = new City*[num_vertices_];

        // Инициализируем каждый список как пустой (nullptr)
        for (int i = 0; i < num_vertices_; ++i) {
            adj_lists_[i] = nullptr;
        }
    }

    // Метод добавления ребра (src → dest): дорога между городами
    void add_edge(int src, int dest) {
        // Создаем новый узел, указывающий на вершину dest: добавляем новый город
        City* new_node = new City(dest);

        // Вставляем его в начало списка src
        new_node->next = adj_lists_[src];
        adj_lists_[src] = new_node;
    }

    // Метод для печати списка смежности графа
    void print_graph() const {
        for (int i = 0; i < num_vertices_; ++i) {
            cout << "Город " << i << ": ";

            City* temp = adj_lists_[i];  // Указатель на начало списка

            // Проходим по списку и печатаем все города, с которыми соединен i
            while (temp) {
                cout << temp->point << " -> ";
                temp = temp->next;
            }

            cout << "Больше городов, связанных с " << i << ", нет" << endl;
        }
    }

    // Деструктор — освобождает всю выделенную память
    ~Graph() {
        for (int i = 0; i < num_vertices_; ++i) {
            City* current = adj_lists_[i];

            // Удаляем каждый узел в списке i
            while (current) {
                City* temp = current;
                current = current->next;
                delete temp;             // Освобождаем память узла
            }
        }

        // Освобождаем массив указателей
        delete[] adj_lists_;
    }
};

// Функция main — пример использования графа
int main() {
    Graph graph(8);  // Создаём граф с 8 городами (0..7)

    // Добавляем ориентированные ребра: дороги между городами
    graph.add_edge(0, 1);
    graph.add_edge(0, 2);
    graph.add_edge(0, 4);
    graph.add_edge(0, 5);
    graph.add_edge(0, 7);
    graph.add_edge(1, 2);
    graph.add_edge(1, 3);
    graph.add_edge(1, 5);
    graph.add_edge(1, 4);
    graph.add_edge(2, 3);
    graph.add_edge(3, 4);
    graph.add_edge(4, 1);
    graph.add_edge(4, 2);
    graph.add_edge(5, 4);
    graph.add_edge(7, 3);
    graph.add_edge(7, 6);

    // Печатаем граф
    graph.print_graph();

    return 0;
}
