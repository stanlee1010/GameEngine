#pragma once

// Root of the game object hierarchy.
//
//                     GameObject                (abstract root)
//          (virtual)/     |      \(virtual)
//   UpdatableObject  DrawableObject  TransformableObject
//          |                \             /
//          |                 ShapeObject             (draw + transform)
//          |              /  |   |   \    \
//          |       Point2D  LineSegment  PolylineObject  CircleObject  EllipseObject
//          |                               |
//          |                          PolygonObject -> RectangleObject, TriangleObject
//          \______________________________________________
//                                     \                     \
//   Player : UpdatableObject + ShapeObject     Bullet : UpdatableObject + CircleObject
//
// Every interface inherits GameObject *virtually*, so a class that combines
// several of them (Player, Bullet) still contains exactly ONE GameObject
// subobject - no diamond problem, no ambiguous calls to isActive() etc.
class GameObject {
public:
    virtual ~GameObject() = default;

    // Pure virtual: every concrete object must say what it is
    virtual const char* getTypeName() const = 0;

    // Methods with a body: state every object shares, same for all types
    bool isActive() const { return m_active; }
    void setActive(bool active) { m_active = active; }

protected:
    GameObject() = default;
    GameObject(const GameObject&) = default;
    GameObject& operator=(const GameObject&) = default;

private:
    bool m_active = true; // inactive objects are skipped by update/draw loops
};
