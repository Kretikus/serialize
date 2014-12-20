#include <QByteArray>
#include <QDebug>
#include <QVector>

#include <stdexcept>

template <typename T>
struct Has_Iterator
{
    template <typename>
    static char test(...);

    template <typename U>
    static int test(typename U::const_iterator*);

    static const bool result = sizeof test<T>(0) != sizeof(char);
};

struct ClassSerializer {
    ClassSerializer(QByteArray & ba) : ba(ba) { ba.append('['); }
    ~ClassSerializer() { ba.append(']'); }

    void append(const QString & str) {
        if (!ba.endsWith('[')) ba.append(',');
        ba.append(str.toLatin1());
    }

    QByteArray & ba;
};

ClassSerializer & operator&(ClassSerializer & dev, int value) {
    dev.append(QString::number(value));
    return dev;
}

ClassSerializer & operator&(ClassSerializer & dev, float value) {
    dev.append(QString::number(value));
    return dev;
}

ClassSerializer & operator&(ClassSerializer & dev, const QString & value) {
    dev.append("\""+value+"\"");
    return dev;
}

template<bool hasIt, typename T>
struct SerializeHelper {
    ClassSerializer & serialize(ClassSerializer & dev, const T & value) {
        for (int i = 0; i < value.size(); ++i) {
            dev & value[i];
        }
        return dev;
    }
};

template<typename T>
struct SerializeHelper<false, T> {
    ClassSerializer & serialize(ClassSerializer & dev, const T & value) {
        QByteArray ba;
        value.serialize(ba);
        dev.append(ba);
        return dev;
    }
};

template<typename T>
ClassSerializer & operator&(ClassSerializer & dev, const T & value) {
    return SerializeHelper<Has_Iterator<T>::result, T>().serialize(dev, value);
}

struct ClassDeSerialize {
    ClassDeSerialize(QByteArray & ba) : ba(ba) {
        checkAndRemove('[', false);
    }
    ~ClassDeSerialize() { checkAndRemove(']', false); }

    void checkAndRemove(char c, bool optional = true) {
        if (!ba.startsWith(c)) {
            if (optional) return;
            throw std::runtime_error("Serialization error!");
        }
        ba.remove(0,1);
    }

    QPair<int,int> nextSimpleType() const {
         QPair<int,int> ret;
         if (ba.startsWith(',')) {
             ret.first = 1;
         } else {
             ret.first = 0;
         }
         int nextTokenEnd = ba.indexOf(',', ret.first);
         if (nextTokenEnd == -1) {
             nextTokenEnd = ba.indexOf(']', ret.first);
         }
         ret.second = nextTokenEnd;
         return ret;
    }

    QByteArray takeNext() {
        QByteArray ret;
        QPair<int,int> nextToken = nextSimpleType();
        if (nextToken.second == -1) {
            throw std::runtime_error("Serialization error! No token found.");
        }
        ret = ba.mid(nextToken.first, nextToken.second - nextToken.first);
        ba.remove(0, nextToken.second);
        return ret;
    }

    QPair<int,int> nextArray() const {
        QPair<int,int> ret;
        if (ba.startsWith(',')) {
            ret.first = 1;
        } else {
            ret.first = 0;
        }
        if (ba.indexOf('[') != ret.first) {
           ret.second = -1;
           return ret;
        }

        int openBrackets = 0;
        int closedBrackets = 0;

        int i = ret.first;
        for (; i < ba.size(); ++i) {
            if (ba[i] == '[') ++openBrackets;
            if (ba[i] == ']') ++closedBrackets;
            if (openBrackets == closedBrackets) break;
        }
        if (openBrackets != closedBrackets) {
            ret.second = -1;
        } else {
            ret.second = i+1;
        }

        return ret;
    }

    QByteArray takeNextArray() {
        QByteArray ret;

        QPair<int,int> nextArrayData = nextArray();
        if (nextArrayData.second == -1) throw std::runtime_error("Serialization error! No array found.");

        ret = ba.mid(nextArrayData.first, nextArrayData.second - nextArrayData.first);
        ba.remove(0, nextArrayData.second);
        return ret;
    }

    bool hasNext() const {
        QPair<int,int> next = nextSimpleType();
        if (next.second == -1) {
            next = nextArray();
        }
        return next.second > 0;
    }

    QByteArray & ba;
};

namespace serialisation {
    template<typename T>
    inline T toValue(const QByteArray & ba);

    template<>
    inline int toValue(const QByteArray & ba) {
        int value;
        QString token(ba);
        bool ok;
        value = token.toInt(&ok);
        if (!ok) {
            throw std::runtime_error("Serialization error. Int convert failed.");
        }
        return value;
    }

    template<>
    inline float toValue(const QByteArray & ba) {
        float value;
        QString token(ba);
        bool ok;
        value = token.toFloat(&ok);
        if (!ok) {
            throw std::runtime_error("Serialization error. Float convert failed.");
        }
        return value;
    }

    template<>
    inline QString toValue(const QByteArray & ba) {
        QString s(ba);
        return s.mid(1, s.size()-2);
    }
}

ClassDeSerialize & operator&(ClassDeSerialize & dev, int & value) {
    value = serialisation::toValue<int>(dev.takeNext());
    return dev;
}

ClassDeSerialize & operator&(ClassDeSerialize & dev, float & value) {
    value = serialisation::toValue<float>(dev.takeNext());
    return dev;
}

ClassDeSerialize & operator&(ClassDeSerialize & dev, QString & value) {
    value = serialisation::toValue<QString>(dev.takeNext());
    return dev;
}

template<typename T>
T getFromContainer(ClassDeSerialize & dev) {
    QByteArray ba = dev.takeNextArray();
    T v;
    v.deserialize(ba);
    return v;
}

template<>
int getFromContainer(ClassDeSerialize & dev) {
    return serialisation::toValue<int>(dev.takeNext());
}

template<bool hasIt, typename T>
struct DeserializeHelper {
    ClassDeSerialize & deserialize(ClassDeSerialize & dev, T & value) {
        while(dev.hasNext()) {
            value.push_back(getFromContainer<typename T::value_type>(dev));
        }
        return dev;
    }
};

template<typename T>
struct DeserializeHelper<false, T> {
    ClassDeSerialize & deserialize(ClassDeSerialize & dev, T & value) {
        QByteArray ba = dev.takeNextArray();
        value.deserialize(ba);
        return dev;
    }
};

template<class T>
ClassDeSerialize & operator&(ClassDeSerialize & dev, T & value) {
    return DeserializeHelper<Has_Iterator<T>::result, T>().deserialize(dev, value);
}
