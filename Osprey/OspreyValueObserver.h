// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020 Darby Johnston, All rights reserved

#pragma once

namespace Osprey
{
    template<typename T>
    class IValueSubject;

    //! This class provides a value observer.
    template<typename T>
    class ValueObserver : public std::enable_shared_from_this<ValueObserver<T> >
    {
    protected:
        void _init(
            const std::weak_ptr<IValueSubject<T> >&,
            const std::function<void(const T&)>&);

        ValueObserver();
        ValueObserver(const ValueObserver&) = delete;
        ValueObserver& operator = (const ValueObserver&) = delete;

    public:
        ~ValueObserver();

        //! Create a new value observer.
        static std::shared_ptr<ValueObserver<T> > create(
            const std::weak_ptr<IValueSubject<T> >&,
            const std::function<void(const T&)>&);

        //! Execute the callback.
        void doCallback(const T&);

    private:
        std::function<void(const T&)> _callback;
        std::weak_ptr<IValueSubject<T> > _subject;
    };

    //! This class provides an interface for a value subject.
    template<typename T>
    class IValueSubject
    {
    public:
        virtual ~IValueSubject() = 0;

        //! Get the value.
        virtual const T& get() const = 0;

        //! Get the number of observers.
        size_t getObserversCount() const;

    protected:
        void _add(const std::weak_ptr<ValueObserver<T> >&);
        void _remove(ValueObserver<T>*);

        std::vector<std::weak_ptr<ValueObserver<T> > > _observers;

        friend class ValueObserver<T>;
    };

    //! This class provides a value subject.
    template<typename T>
    class ValueSubject : public IValueSubject<T>
    {
    protected:
        ValueSubject();
        explicit ValueSubject(const T&);
        ValueSubject(const ValueSubject&) = delete;
        ValueSubject& operator = (const ValueSubject&) = delete;

    public:
        //! Create a new value subject.
        static std::shared_ptr<ValueSubject<T> > create();

        //! Create a new value subject with the given value.
        static std::shared_ptr<ValueSubject<T> > create(const T&);

        //! Set the value.
        void setAlways(const T&);

        //! Set the value only if it has changed.
        bool setIfChanged(const T&);

        const T& get() const override;

    private:
        T _value = T();
    };

    template<typename T>
    inline void ValueObserver<T>::_init(const std::weak_ptr<IValueSubject<T> >& value, const std::function<void(const T&)>& callback)
    {
        _subject = value;
        _callback = callback;
        if (auto subject = value.lock())
        {
            subject->_add(ValueObserver<T>::shared_from_this());
            _callback(subject->get());
        }
    }

    template<typename T>
    inline ValueObserver<T>::ValueObserver()
    {}

    template<typename T>
    inline ValueObserver<T>::~ValueObserver()
    {
        if (auto subject = _subject.lock())
        {
            subject->_remove(this);
        }
    }

    template<typename T>
    inline void ValueObserver<T>::doCallback(const T& value)
    {
        _callback(value);
    }

    template<typename T>
    inline IValueSubject<T>::~IValueSubject()
    {}

    template<typename T>
    inline size_t IValueSubject<T>::getObserversCount() const
    {
        return _observers.size();
    }

    template<typename T>
    inline void IValueSubject<T>::_add(const std::weak_ptr<ValueObserver<T> >& observer)
    {
        _observers.push_back(observer);
    }

    template<typename T>
    inline void IValueSubject<T>::_remove(ValueObserver<T>* observer)
    {
        auto i = _observers.begin();
        while (i != _observers.end())
        {
            bool erase = false;
            if (auto j = i->lock())
            {
                if (observer == j.get())
                {
                    erase = true;
                }
            }
            else
            {
                erase = true;
            }
            if (erase)
            {
                i = _observers.erase(i);
            }
            else
            {
                ++i;
            }
        }
    }

    template<typename T>
    inline std::shared_ptr<ValueObserver<T> > ValueObserver<T>::create(const std::weak_ptr<IValueSubject<T> >& value, const std::function<void(const T&)>& callback)
    {
        std::shared_ptr<ValueObserver<T> > out(new ValueObserver<T>);
        out->_init(value, callback);
        return out;
    }

    template<typename T>
    inline ValueSubject<T>::ValueSubject()
    {}

    template<typename T>
    inline ValueSubject<T>::ValueSubject(const T& value) :
        _value(value)
    {}

    template<typename T>
    inline std::shared_ptr<ValueSubject<T> > ValueSubject<T>::create()
    {
        return std::shared_ptr<ValueSubject<T> >(new ValueSubject<T>);
    }

    template<typename T>
    inline std::shared_ptr<ValueSubject<T> > ValueSubject<T>::create(const T& value)
    {
        return std::shared_ptr<ValueSubject<T> >(new ValueSubject<T>(value));
    }

    template<typename T>
    inline void ValueSubject<T>::setAlways(const T& value)
    {
        _value = value;
        for (const auto& s : IValueSubject<T>::_observers)
        {
            if (auto observer = s.lock())
            {
                observer->doCallback(_value);
            }
        }
    }

    template<typename T>
    inline bool ValueSubject<T>::setIfChanged(const T& value)
    {
        if (value == _value)
            return false;
        _value = value;
        for (const auto& s : IValueSubject<T>::_observers)
        {
            if (auto observer = s.lock())
            {
                observer->doCallback(_value);
            }
        }
        return true;
    }

    template<typename T>
    inline const T& ValueSubject<T>::get() const
    {
        return _value;
    }

} // namespace Osprey
