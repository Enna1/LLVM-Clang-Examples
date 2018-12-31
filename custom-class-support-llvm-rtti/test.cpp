#include "llvm/Support/Casting.h"
#include <iostream>
#include <vector>
using namespace llvm;

class Shape
{
public:
    // 类似class Value 中 enum ValueTy的定义
    enum ShapeKind
    {
        /* Square Kind Begin */
        SK_SQUARE,
        SK_SEPCIALSQUARE,
        /* Square Kind end */
        SK_CIRCLE,
    };

private:
    const ShapeKind kind_;

public:
    Shape(ShapeKind kind) : kind_(kind) {}

    ShapeKind getKind() const
    {
        return kind_;
    }

    virtual double computeArea() = 0;
};

class Square : public Shape
{
public:
    double side_length_;

public:
    Square(double side_length) : Shape(SK_SQUARE), side_length_(side_length) {}

    Square(ShapeKind kind, double side_length)
        : Shape(kind), side_length_(side_length)
    {
    }

    double computeArea() override
    {
        return side_length_ * side_length_;
    }

    static bool classof(const Shape *s)
    {
        return s->getKind() >= SK_SQUARE && s->getKind() <= SK_SEPCIALSQUARE;
    }
};

class SepcialSquare : public Square
{
public:
    double another_side_length_;

public:
    SepcialSquare(double side_length, double another_side_length)
        : Square(SK_SEPCIALSQUARE, side_length),
          another_side_length_(another_side_length)
    {
    }

    double computeArea() override
    {
        return side_length_ * another_side_length_;
    }

    static bool classof(const Shape *s)
    {
        return s->getKind() == SK_SEPCIALSQUARE;
    }
};

class Circle : public Shape
{
public:
    double radius_;

public:
    Circle(double radius) : Shape(SK_CIRCLE), radius_(radius) {}

    double computeArea() override
    {
        return 3.14 * radius_ * radius_;
    }

    static bool classof(const Shape *s)
    {
        return s->getKind() == SK_CIRCLE;
    }
};

int main()
{
    Square s1(1);
    SepcialSquare s2(1, 2);
    Circle s3(3);
    std::vector<Shape *> v{ &s1, &s2, &s3 };
    for (auto i : v)
    {
        if (auto *S = dyn_cast<Square>(i))
        {
            std::cout << "This is a Square object\n";
            std::cout << "Area is : " << S->computeArea() << "\n";
        }
        if (auto *SS = dyn_cast<SepcialSquare>(i))
        {
            std::cout << "This is a SepcialSquare object\n";
            std::cout << "Area is : " << SS->computeArea() << "\n";
        }
        if (auto *C = dyn_cast<Circle>(i))
        {
            std::cout << "This is a Circle object\n";
            std::cout << "Area is : " << C->computeArea() << "\n";
        }
        std::cout << "-----\n";
    }

    return 0;
}