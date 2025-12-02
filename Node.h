/**
 * @file Node.h
 * @brief 节点类头文件，定义可视化节点的基本属性和行为
 * @author 
 * @version 1.0.0
 * @date 2024
 */

#ifndef NODE_H
#define NODE_H

#include <QGraphicsItem>             // Qt图形项基类
#include <QPainter>                   // Qt绘图类
#include <QJsonObject>               // JSON对象类
#include <QDebug>                    // 调试输出类
#include <QColor>                    // 颜色类

// 调试开关：设置为true启用端口调试输出
static const bool DEBUG_PORTS = true;

// 前向声明
class Connection;                     // 连接线类

/**
 * @class Node
 * @brief 节点类，继承自QGraphicsItem
 * 
 * 该类表示节点编辑器中的可视化节点，支持：
 * - 节点的绘制和显示
 * - 节点的拖拽移动
 * - 节点的选择和高亮
 * - 与其他节点的连接管理
 * - 节点属性的序列化和反序列化
 */
class Node : public QGraphicsItem
{
public:
    /**
     * @brief 自定义类型标识符，用于区分不同的图形项
     */
    enum { Type = UserType + 1 };
    
    /**
     * @brief 构造函数
     * @param type 节点类型（如signal_source、filter等）
     * @param name 节点显示名称
     * @param position 节点在场景中的初始位置
     */
    Node(const QString &type, const QString &name, const QPointF &position);
    
    /**
     * @brief 析构函数，负责清理相关资源
     */
    ~Node();
    
    /**
     * @brief 返回节点的自定义类型
     * @return UserType + 1
     */
    int type() const override { return Type; }
    
    /**
     * @brief 返回节点的边界矩形
     * @return 节点的边界矩形
     */
    QRectF boundingRect() const override;
    
    /**
     * @brief 绘制节点的外观
     * @param painter 绘制器对象
     * @param option 样式选项
     * @param widget 父控件指针
     */
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr)  override;
    
    // 节点属性访问器
    QString getType() const { return m_type; }           // 获取节点类型
    QString getName() const { return m_name; }           // 获取节点名称
    QStringList getParameters() const { return m_parameters; }  // 获取节点参数列表
    
    // 节点属性设置器（设置后自动触发重绘）
    void setType(const QString &type);       // 设置节点类型
    void setName(const QString &name);       // 设置节点名称
    void setParameters(const QStringList &params);  // 设置节点参数
    void setCustomColor(const QColor &color);       // 设置自定义颜色
    void setDisplayTypeName(const QString &name);   // 设置显示的类型名称
    
    // 获取自定义属性
    QColor getCustomColor() const { return m_customColor; }  // 获取自定义颜色
    bool hasCustomColor() const { return m_useCustomColor; }  // 是否使用自定义颜色
    QString getDisplayTypeName() const { return m_displayTypeName; }  // 获取显示的类型名称
    
    // 端口数量管理
    int getInputPortCount() const { return m_inputPortCount; }   // 获取输入端口数量
    int getOutputPortCount() const { return m_outputPortCount; } // 获取输出端口数量
    void setInputPortCount(int count);   // 设置输入端口数量（自动触发重绘）
    void setOutputPortCount(int count);  // 设置输出端口数量（自动触发重绘）
    
    /**
     * @brief 添加与该节点相关的连接
     * @param connection 连接线指针
     */
    void addConnection(Connection *connection);
    
    /**
     * @brief 移除与该节点相关的连接
     * @param connection 连接线指针
     */
    void removeConnection(Connection *connection);
    
    /**
     * @brief 获取与该节点相关的所有连接
     * @return 连接列表
     */
    QList<Connection*> getConnections() const { return m_connections; }
    
    /**
     * @brief 获取输入端口在场景中的位置（兼容单端口）
     * @return 第一个输入端口的场景坐标
     */
    QPointF getInputPortPos() const;
    
    /**
     * @brief 获取输出端口在场景中的位置（兼容单端口）
     * @return 第一个输出端口的场景坐标
     */
    QPointF getOutputPortPos() const;
    
    /**
     * @brief 获取指定索引的输入端口位置
     * @param portIndex 端口索引（从0开始）
     * @return 输入端口的场景坐标
     */
    QPointF getInputPortPos(int portIndex) const;
    
    /**
     * @brief 获取指定索引的输出端口位置
     * @param portIndex 端口索引（从0开始）
     * @return 输出端口的场景坐标
     */
    QPointF getOutputPortPos(int portIndex) const;
    
    /**
     * @brief 检查点是否在输入端口的连接范围内
     * @param point 要检查的点（场景坐标）
     * @return 如果点在输入端口范围内返回true
     */
    bool isPointAtInputPort(const QPointF &point) const;
    
    /**
     * @brief 检查点是否在输出端口的连接范围内
     * @param point 要检查的点（场景坐标）
     * @return 如果点在输出端口范围内返回true
     */
    bool isPointAtOutputPort(const QPointF &point) const;
    
    /**
     * @brief 获取鼠标位置对应的输入端口索引
     * @param point 要检查的点（场景坐标）
     * @return 端口索引，如果不在任何端口范围内返回-1
     */
    int getInputPortIndexAt(const QPointF &point) const;
    
    /**
     * @brief 获取鼠标位置对应的输出端口索引
     * @param point 要检查的点（场景坐标）
     * @return 端口索引，如果不在任何端口范围内返回-1
     */
    int getOutputPortIndexAt(const QPointF &point) const;
    
    /**
     * @brief 设置输入端口的高亮状态
     * @param highlighted 是否高亮
     */
    void setInputPortHighlighted(bool highlighted);
    
    /**
     * @brief 设置输出端口的高亮状态
     * @param highlighted 是否高亮
     */
    void setOutputPortHighlighted(bool highlighted);
    
    /**
     * @brief 绘制端口（单独绘制以确保在节点框之上）
     * @param painter 绘制器对象
     * 
     * 虚函数，子类可以重写以自定义端口绘制
     */
    virtual void drawPorts(QPainter *painter);
    
    /**
     * @brief 将节点数据转换为JSON对象
     * @return 包含节点信息的JSON对象
     */
    QJsonObject toJson() const;
    
    /**
     * @brief 从JSON对象创建节点实例
     * @param json 包含节点信息的JSON对象
     * @return 新创建的节点指针
     */
    static Node* fromJson(const QJsonObject &json);
    
protected:
    /**
     * @brief 鼠标按下事件处理
     * @param event 鼠标事件对象
     */
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    
    /**
     * @brief 鼠标移动事件处理
     * @param event 鼠标事件对象
     */
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
    
    /**
     * @brief 鼠标释放事件处理
     * @param event 鼠标事件对象
     */
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
    
    /**
     * @brief 图形项属性变化事件处理
     * @param change 变化类型
     * @param value 新值
     * @return 变化后的值
     */
    QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;
    
private:
    QString m_type;                       // 节点类型
    QString m_name;                       // 节点显示名称
    QStringList m_parameters;             // 节点参数列表
    QPointF m_dragStartPos;               // 拖拽起始位置
    
    QList<Connection*> m_connections;      // 与该节点相关的连接列表
    
    bool m_inputPortHighlighted;          // 输入端口高亮状态
    bool m_outputPortHighlighted;         // 输出端口高亮状态
    
    QColor m_customColor;                 // 自定义颜色
    bool m_useCustomColor;                // 是否使用自定义颜色
    QString m_displayTypeName;            // 显示的类型名称
    
    int m_inputPortCount;                 // 输入端口数量
    int m_outputPortCount;                // 输出端口数量
    
    /**
     * @brief 调整大小区域枚举
     */
    enum ResizeHandle {
        NoHandle = 0,
        TopLeft,
        TopRight,
        BottomLeft,
        BottomRight,
        Left,
        Right,
        Top,
        Bottom
    };
    

    
public:
    // 尺寸访问器和设置器
    qreal getWidth() const { return m_width; }
    qreal getHeight() const { return m_height; }
    void setSize(qreal width, qreal height);
    // 节点尺寸默认值和常量（公开以便外部访问）
    static const int DEFAULT_WIDTH = 120;   // 默认节点宽度
    static const int DEFAULT_HEIGHT = 70;   // 默认节点高度
    static const int MIN_WIDTH = 80;        // 最小宽度
    static const int MIN_HEIGHT = 50;       // 最小高度
    static const int MAX_WIDTH = 300;       // 最大宽度
    static const int MAX_HEIGHT = 200;      // 最大高度
    static const int PORT_RADIUS = 8;       // 端点半径
    static const int PORT_CAPTURE_RADIUS = 12; // 端点捕获范围
    static const int HANDLE_SIZE = 8;       // 调整把手大小
    
    // 兼容旧代码的静态常量
    static const int WIDTH = DEFAULT_WIDTH;
    static const int HEIGHT = DEFAULT_HEIGHT;

private:
    /**
     * @brief 获取鼠标位置对应的调整把手
     * @param pos 本地坐标
     * @return 调整把手枚举
     */
    ResizeHandle getResizeHandleAt(const QPointF &pos) const;
    
    /**
     * @brief 绘制调整把手
     * @param painter 绘制器
     */
    void drawResizeHandles(QPainter *painter);
    
    qreal m_width;                        // 节点当前宽度
    qreal m_height;                       // 节点当前高度
    bool m_resizing;                      // 是否正在调整大小
    ResizeHandle m_currentHandle;         // 当前激活的调整把手
    QPointF m_resizeStartPos;             // 调整大小起始位置
    QSizeF m_resizeStartSize;             // 调整大小起始尺寸
};

#endif // NODE_H
