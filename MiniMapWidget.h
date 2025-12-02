/**
 * @file MiniMapWidget.h
 * @brief 导航小地图组件头文件
 * @author
 * @version 1.0.0
 * @date 2024
 */

#ifndef MINIMAPWIDGET_H
#define MINIMAPWIDGET_H

#include <QWidget>
#include <QGraphicsView>

// 前向声明
class QGraphicsScene;
class NodeView;

/**
 * @class MiniMapWidget
 * @brief 导航小地图组件，显示场景缩略图和当前视口位置
 *
 * 该组件提供：
 * - 显示整个场景的缩略图
 * - 高亮显示当前视口区域
 * - 点击小地图快速导航到对应位置
 * - 拖拽视口框移动视图
 */
class MiniMapWidget : public QWidget
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param parent 父控件指针
     */
    explicit MiniMapWidget(QWidget *parent = nullptr);

    /**
     * @brief 设置要显示的场景和主视图
     * @param scene 图形场景
     * @param mainView 主视图
     */
    void setSceneAndView(QGraphicsScene *scene, NodeView *mainView);

    /**
     * @brief 更新小地图显示
     */
    void updateMiniMap();

protected:
    /**
     * @brief 绘制事件
     * @param event 绘制事件对象
     */
    void paintEvent(QPaintEvent *event) override;

    /**
     * @brief 鼠标按下事件
     * @param event 鼠标事件对象
     */
    void mousePressEvent(QMouseEvent *event) override;

    /**
     * @brief 鼠标移动事件
     * @param event 鼠标事件对象
     */
    void mouseMoveEvent(QMouseEvent *event) override;

    /**
     * @brief 鼠标释放事件
     * @param event 鼠标事件对象
     */
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    /**
     * @brief 将小地图坐标转换为场景坐标
     * @param widgetPos 小地图坐标
     * @return 场景坐标
     */
    QPointF widgetToScene(const QPointF &widgetPos) const;

    /**
     * @brief 将场景坐标转换为小地图坐标
     * @param scenePos 场景坐标
     * @return 小地图坐标
     */
    QPointF sceneToWidget(const QPointF &scenePos) const;

    /**
     * @brief 获取缩放后的场景边界矩形
     * @return 边界矩形
     */
    QRectF getSceneBounds() const;

    QGraphicsScene *m_scene;     ///< 图形场景指针
    NodeView *m_mainView;        ///< 主视图指针
    bool m_dragging;             ///< 是否正在拖拽
    QPointF m_dragStartPos;      ///< 拖拽起始位置
};

#endif // MINIMAPWIDGET_H

