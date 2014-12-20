#include "serialize.h"

class TestClass
{
public:
    TestClass() : a(), b() {}
    TestClass(int a, float b, const QString & c) : a(a), b(b), c(c) {}

    bool operator==(const TestClass & rhs) const {
        return a == rhs.a && b == rhs.b && c == rhs.c;
    }
     bool operator!=(const TestClass & rhs) const {
         return !operator ==(rhs);
     }

    virtual void serialize(QByteArray & outArchive) const {
        ClassSerializer(outArchive) & a & b & c;
    }

    virtual void deserialize(QByteArray & inArchive) {
        ClassDeSerialize(inArchive) & a & b & c;
    }

     QString toString() const {
         return "a: " + QString::number(a) + " b: " + QString::number(a) + " c: " + c;
     }

private:
    int a;
    float b;
    QString c;
};

class NestedClasses {

public:
    NestedClasses() {}
    NestedClasses(const TestClass & a, const TestClass & b) : a(a), b(b) {}

    bool operator==(const NestedClasses & rhs) const {
        return a == rhs.a && b == rhs.b;
    }
    bool operator!=(const NestedClasses & rhs) const {
     return !operator ==(rhs);
    }

    virtual void serialize(QByteArray & outArchive) const {
        ClassSerializer(outArchive) & a & b;
    }

    virtual void deserialize(QByteArray & inArchive) {
        ClassDeSerialize(inArchive) & a & b;
    }

private:
    TestClass a;
    TestClass b;
};


#include <QDebug>

int main(int argc, char *argv[])
{
    QByteArray ba;
    TestClass tclass(44,3.4f,"TADA");
    tclass.serialize(ba);

    qDebug() << QString(ba);

    {
        TestClass deSerialized;
        deSerialized.deserialize(ba);

        if (tclass != deSerialized) {
            qDebug() << "Deserialisation failed";
            qDebug() << deSerialized.toString();
        }
    }
    ba.clear();
    NestedClasses nested(TestClass(11,1.1f,"11.11"), TestClass(22,2.3f,"44.44"));
    nested.serialize(ba);

    qDebug() << QString(ba);

    {
        NestedClasses deSerialized;
        deSerialized.deserialize(ba);

        if (nested != deSerialized) {
            qDebug() << "Deserialisation failed";
        }
    }

    QVector<NestedClasses> vec;
    {
        NestedClasses nested1(TestClass(11,1.1f,"11.11"), TestClass(22,2.3f,"44.44"));
        NestedClasses nested2(TestClass(55,5.6f,"61.61"), TestClass(82,10.3f,"99.44"));
        vec << nested1 << nested2;
    }
    ba.clear();
    ClassSerializer(ba) & vec;
    qDebug() << QString(ba);

    {
        QVector<NestedClasses> deVec;
        ClassDeSerialize(ba) & deVec;

        qDebug() << "Size: " << deVec.size();
    }

    QList<int> vec2;
    vec2 << 2 << 5;
    ba.clear();
    ClassSerializer(ba) & vec2;
    qDebug() << "Vector" << QString(ba);

    {
        QList<int> deVec;
        ClassDeSerialize(ba) & deVec;

        qDebug() << "Size: " << deVec.size();
    }


    QVector<QVector<int> > vec3;
    vec3 << (QVector<int>() << 2 << 3);
    vec3 << (QVector<int>() << 10 << 11);
    ba.clear();
    ClassSerializer(ba) & vec3;
    qDebug() << "Vector" << QString(ba) << vec3;

}

