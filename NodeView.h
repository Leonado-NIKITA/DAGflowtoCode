/**
 * @file NodeView.h
 * @brief 节点视图类头文件，提供场景的可视化视图和交互功能
 * @author 
 * @version 1.0.0
 * @date 2024
 */

#ifndef NODEVIEW_H
#define NODEVIEW_H

#include <QGraphicsView>               // Qt图形视图基类

// 前向声明
class NodeScene;                       // 节点场景类
class MiniMapWidget;                   // 导航小地图组件

/**
 * @class NodeView
 * @brief 节点视图类，继承自QGraphicsView
 * 
 * 该类提供节点场景的可视化显示和用户交互功能：
 * - 支持鼠标滚轮缩放
 * - 绘制网格背景
 * - 支持键盘快捷键
 * - 提供平滑的视图变换效果
 */
class NodeView : public QGraphicsView
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param scene 要显示的节点场景
     * @param parent 父控件指针，默认为nullptr
     */
    NodeView(NodeScene *scene, QWidget *parent = nullptr);
    
protected:
    /**
     * @brief 鼠标滚轮事件处理
     * @param event 鼠标滚轮事件对象
     * 
     * 支持使用鼠标滚轮进行视图缩放
     */
    void wheelEvent(QWheelEvent *event) override;
    
    /**
     * @brief 背景绘制事件处理
     * @param painter 绘制器对象
     * @param rect 需要重绘的矩形区域
     * 
     * 绘制深色背景和网格线，提供更好的视觉效果
     */
    void drawBackground(QPainter *painter, const QRectF &rect) override;
    
    /**
     * @brief 键盘按键事件处理
     * @param event 键盘事件对象
     * 
     * 支持各种键盘快捷键操作
     */
    void keyPressEvent(QKeyEvent *event) override;
    
    /**
     * @brief 窗口大小改变事件处理
     * @param event 大小改变事件对象
     */
    void resizeEvent(QResizeEvent *event) override;
    
    /**
     * @brief 鼠标按下事件处理
     * @param event 鼠标事件对象
     * 
     * 右键按下开始拖动画布
     */
    void mousePressEvent(QMouseEvent *event) override;
    
    /**
     * @brief 鼠标移动事件处理
     * @param event 鼠标事件对象
     * 
     * 右键拖动时移动画布
     */
    void mouseMoveEvent(QMouseEvent *event) override;
    
    /**
     * @brief 鼠标释放事件处理
     * @param event 鼠标事件对象
     * 
     * 右键释放停止拖动画布
     */
    void mouseReleaseEvent(QMouseEvent *event) override;
    
    /**
     * @brief 拖拽进入事件处理
     * @param event 拖拽事件对象
     * 
     * 检查拖拽的数据是否为节点类型
     */
    void dragEnterEvent(QDragEnterEvent *event) override;
    
    /**
     * @brief 拖拽移动事件处理
     * @param event 拖拽事件对象
     */
    void dragMoveEvent(QDragMoveEvent *event) override;
    
    /**
     * @brief 拖拽释放事件处理
     * @param event 拖拽事件对象
     * 
     * 在释放位置创建新节点
     */
    void dropEvent(QDropEvent *event) override;
    
    /**
     * @brief 右键菜单事件处理
     * @param event 上下文菜单事件对象
     * 
     * 显示右键上下文菜单，包含打包、拆分等操作
     */
    void contextMenuEvent(QContextMenuEvent *event) override;

private:
    /**
     * @brief 缩放视图
     * @param scaleFactor 缩放因子
     * 
     * 安全地缩放视图，确保缩放范围在合理区间内
     */
    void scaleView(qreal scaleFactor);
    
    /**
     * @brief 更新小地图位置
     */
    void updateMiniMapPosition();

    NodeScene *m_scene;          ///< 关联的节点场景指针
    MiniMapWidget *m_miniMap;    ///< 导航小地图组件
    
    bool m_isPanning;            ///< 是否正在拖动画布
    QPoint m_panStartPos;        ///< 拖动起始位置
};

#endif // NODEVIEW_H