/**
 * @file Connection.h
 * @brief 连接线类头文件，定义节点之间的连接关系
 * @author 
 * @version 1.0.0
 * @date 2024
 */

#ifndef CONNECTION_H
#define CONNECTION_H

#include <QGraphicsPathItem>           // Qt路径图形项类
#include <QDebug>                      // 调试输出类

// 前向声明
class Node;                            // 节点类

// 调试开关：设置为true启用连接线调试输出
static const bool DEBUG_CONNECTION_PATH = true;

/**
 * @class Connection
 * @brief 连接线类，继承自QGraphicsPathItem
 * 
 * 该类表示两个节点之间的连接关系：
 * - 支持多种绘制方式（直线、贝塞尔曲线、直角线）
 * - 自动跟随节点的移动更新路径
 * - 支持序列化和反序列化
 * - 管理与源节点和目标节点的关联关系
 * - 记录具体的端口索引
 */
class Connection : public QGraphicsPathItem
{
public:
    /**
     * @brief 自定义类型标识符，用于区分不同的图形项
     */
    enum { Type = UserType + 2 };
    
    /**
     * @brief 连线绘制类型枚举
     */
    enum LineType {
        Bezier = 0,     ///< 贝塞尔曲线（默认）
        Straight,       ///< 直线
        Orthogonal      ///< 直角线（折线）
    };
    
    /**
     * @brief 构造函数，创建两个节点之间的连接（兼容旧版本，使用第一个端口）
     * @param fromNode 源节点（输出端）
     * @param toNode 目标节点（输入端）
     */
    Connection(Node *fromNode, Node *toNode);
    
    /**
     * @brief 构造函数，创建两个节点指定端口之间的连接
     * @param fromNode 源节点（输出端）
     * @param fromPortIndex 源节点的输出端口索引
     * @param toNode 目标节点（输入端）
     * @param toPortIndex 目标节点的输入端口索引
     */
    Connection(Node *fromNode, int fromPortIndex, Node *toNode, int toPortIndex);
    
    /**
     * @brief 析构函数，清理相关资源
     */
    ~Connection();
    
    /**
     * @brief 返回连接线的自定义类型
     * @return UserType + 2
     */
    int type() const override { return Type; }
    
    // 节点访问器
    Node* getFromNode() const { return m_fromNode; }  ///< 获取源节点
    Node* getToNode() const { return m_toNode; }      ///< 获取目标节点
    
    // 端口索引访问器
    int getFromPortIndex() const { return m_fromPortIndex; }  ///< 获取源端口索引
    int getToPortIndex() const { return m_toPortIndex; }      ///< 获取目标端口索引
    
    // 线型访问器和设置器
    LineType getLineType() const { return m_lineType; }       ///< 获取连线类型
    void setLineType(LineType type);                          ///< 设置连线类型
    
    /**
     * @brief 获取线型的显示名称
     * @param type 线型枚举值
     * @return 线型的中文名称
     */
    static QString lineTypeName(LineType type);
    
    /**
     * @brief 更新连接线的路径
     * 
     * 当节点位置发生变化时调用此方法重新计算连接线路径
     */
    void updatePath();
    
    /**
     * @brief 打印连接线状态信息（仅在调试模式下有效）
     */
    void printStatus() const;
    
    /**
     * @brief 将连接线数据转换为JSON对象
     * @return 包含连接信息的JSON对象
     */
    QJsonObject toJson() const;
    
private:
    /**
     * @brief 初始化连接
     */
    void initConnection();
    
    Node *m_fromNode;       ///< 源节点指针
    Node *m_toNode;         ///< 目标节点指针
    int m_fromPortIndex;    ///< 源节点的输出端口索引
    int m_toPortIndex;      ///< 目标节点的输入端口索引
    LineType m_lineType;    ///< 连线绘制类型
};

#endif // CONNECTION_H