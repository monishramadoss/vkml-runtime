#include <unordered_map>
#include <vector>
#include <memory>
#include <algorithm>

class Dag
{
    struct Node
    {
        uint64_t id;
        std::vector<std::shared_ptr<Node>> dependencies;
        std::vector<std::shared_ptr<Node>> dependents;

        Node(uint64_t id) : id(id) {}
    };

    std::unordered_map<uint64_t, std::shared_ptr<Node>> m_nodes;

public:
    void addNode(uint64_t id)
    {
        if (m_nodes.find(id) == m_nodes.end())
        {
            m_nodes[id] = std::make_shared<Node>(id);
        }
    }

    void addDependency(uint64_t id, uint64_t dependencyId)
    {
        auto node = m_nodes[id];
        auto dependency = m_nodes[dependencyId];
        node->dependencies.push_back(dependency);
        dependency->dependents.push_back(node);
    }

    std::vector<uint64_t> getExecutionOrder() const
    {
        std::vector<uint64_t> order;
        std::unordered_map<uint64_t, bool> visited;

        for (const auto &pair : m_nodes)
        {
            if (!visited[pair.first])
            {
                visit(pair.second, visited, order);
            }
        }

        std::reverse(order.begin(), order.end());
        return order;
    }

private:
    void visit(const std::shared_ptr<Node> &node, std::unordered_map<uint64_t, bool> &visited, std::vector<uint64_t> &order) const
    {
        if (visited[node->id])
        {
            return;
        }

        visited[node->id] = true;

        for (const auto &dependency : node->dependencies)
        {
            visit(dependency, visited, order);
        }

        order.push_back(node->id);
    }
};