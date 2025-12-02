/**
 * @file GroupNode.h
 * @brief 组节点类头文件，用于将多个节点打包成一个节点
 * @author
 * @version 1.0.0
 * @date 2024
 */

#ifndef GROUPNODE_H
#define GROUPNODE_H

#include "Node.h"
#include "Connection.h"
#include <QList>
#include <QMap>

/**
 * @struct PortMapping
 * @brief 端口映射信息，记录组节点端口与内部节点端口的对应关系
 */
struct PortMapping {
    Node *internalNode;      ///< 内部节点指针
    int internalPortIndex;   ///< 内部端口索引
    bool isInput;            ///< 是否为输入端口
    QString portLabel;       ///< 端口标签（用于显示）
};

/**
 * @struct ExternalConnection
 * @brief 外部连接信息，记录打包前与外部节点的连接
 */
struct ExternalConnection {
    Node *externalNode;      ///< 外部节点指针
    int externalPortIndex;   ///< 外部端口索引
    Node *internalNode;      ///< 内部节点指针
    int internalPortIndex;   ///< 内部端口索引
    bool isInput;            ///< 对于组节点来说是输入还是输出
    Connection *originalConnection; ///< 原始连接指针
};

/**
 * @class GroupNode
 * @brief 组节点类，继承自Node
 * 
 * 该类表示一个打包后的节点，包含：
 * - 内部节点列表
 * - 内部连接列表
 * - 端口映射（悬空端口到组端口的映射）
 * - 支持拆分还原
 */
class GroupNode : public Node
{
public:
    /**
     * @brief 自定义类型标识符
     * 注意：Node=UserType+1, Connection=UserType+2, GroupNode=UserType+3
     */
    enum { Type = UserType + 3 };
    
    /**
     * @brief 构造函数
     * @param name 组节点名称
     * @param position 组节点位置
     */
    GroupNode(const QString &name, const QPointF &position);
    
    /**
     * @brief 析构函数
     */
    ~GroupNode();
    
    /**
     * @brief 返回组节点的自定义类型
     * @return UserType + 2
     */
    int type() const override { return Type; }
    
    /**
     * @brief 绘制组节点
     */
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;
    
    /**
     * @brief 绘制端口（重写以显示端口来源标签）
     */
    void drawPorts(QPainter *painter);
    
    /**
     * @brief 设置内部节点
     * @param nodes 要打包的节点列表
     */
    void setInternalNodes(const QList<Node*> &nodes);
    
    /**
     * @brief 设置内部连接
     * @param connections 内部连接列表
     */
    void setInternalConnections(const QList<Connection*> &connections);
    
    /**
     * @brief 设置外部连接信息
     * @param connections 外部连接信息列表
     */
    void setExternalConnections(const QList<ExternalConnection> &connections);
    
    /**
     * @brief 获取内部节点列表
     * @return 内部节点列表
     */
    QList<Node*> getInternalNodes() const { return m_internalNodes; }
    
    /**
     * @brief 获取内部连接列表
     * @return 内部连接列表
     */
    QList<Connection*> getInternalConnections() const { return m_internalConnections; }
    
    /**
     * @brief 获取外部连接信息
     * @return 外部连接信息列表
     */
    QList<ExternalConnection> getExternalConnections() const { return m_externalConnections; }
    
    /**
     * @brief 获取输入端口映射
     * @return 输入端口映射列表
     */
    QList<PortMapping> getInputPortMappings() const { return m_inputPortMappings; }
    
    /**
     * @brief 获取输出端口映射
     * @return 输出端口映射列表
     */
    QList<PortMapping> getOutputPortMappings() const { return m_outputPortMappings; }
    
    /**
     * @brief 计算并设置端口映射
     * 根据悬空端口（与外部节点连接的端口）计算组节点的输入输出端口
     */
    void calculatePortMappings();
    
    /**
     * @brief 获取内部节点的原始位置
     * @return 节点到位置的映射
     */
    QMap<Node*, QPointF> getOriginalPositions() const { return m_originalPositions; }
    
    /**
     * @brief 设置内部节点的原始位置
     * @param positions 节点到位置的映射
     */
    void setOriginalPositions(const QMap<Node*, QPointF> &positions);
    
    /**
     * @brief 检查是否为组节点
     * @return 始终返回true
     */
    bool isGroupNode() const { return true; }
    
    /**
     * @brief 获取组件等级
     * @return 组件等级（默认1级）
     */
    int getGroupLevel() const { return m_groupLevel; }
    
    /**
     * @brief 设置组件等级
     * @param level 组件等级
     */
    void setGroupLevel(int level);
    
    /**
     * @brief 将组节点数据转换为JSON对象
     * @return 包含组节点信息的JSON对象
     */
    QJsonObject toJson() const;
    
    /**
     * @brief 从JSON对象创建组节点实例
     * @param json 包含组节点信息的JSON对象
     * @param allNodes 所有节点的映射（用于恢复内部节点引用）
     * @return 新创建的组节点指针
     */
    static GroupNode* fromJson(const QJsonObject &json, const QMap<QString, Node*> &allNodes);

private:
    QList<Node*> m_internalNodes;              ///< 内部节点列表
    QList<Connection*> m_internalConnections;  ///< 内部连接列表
    QList<ExternalConnection> m_externalConnections; ///< 外部连接信息
    
    QList<PortMapping> m_inputPortMappings;    ///< 输入端口映射
    QList<PortMapping> m_outputPortMappings;   ///< 输出端口映射
    
    QMap<Node*, QPointF> m_originalPositions;  ///< 内部节点的原始位置
    
    int m_groupLevel;                          ///< 组件等级（默认1级）
};

#endif // GROUPNODE_H

