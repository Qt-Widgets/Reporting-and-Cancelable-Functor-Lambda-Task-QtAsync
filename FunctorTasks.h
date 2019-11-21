#ifndef FUNCTORTASKS_H
#define FUNCTORTASKS_H

#include <QFuture>
#include <QFutureInterfaceBase>
#include <QtConcurrent>

class OperationToken
{
public:
    OperationToken(QFutureInterfaceBase* f = nullptr) : m_f(f)  {   }

    bool isCancellationRequested() const
    {
        return m_f != nullptr && m_f->isCanceled();
    }

    void setProgressValue(int value) const
    {
        if(m_f != nullptr)
            m_f->setProgressValue(value);
    }

    void setProgressRange(int minimum, int maximum) const
    {
        if(m_f != nullptr)
            m_f->setProgressRange(minimum, maximum);
    }

private:
    QFutureInterfaceBase* m_f;
};

/*== functor task ==*/
template <typename T, typename Functor>
class RunCancelableFunctorTask : public QtConcurrent::RunFunctionTask<T>
{
public:
    RunCancelableFunctorTask(Functor func) : m_func(func) { }
    void runFunctor() override
    {
        OperationToken token(this);
        this->result = m_func(token);
    }
private:
    Functor m_func;
};

template <typename Functor>
class RunCancelableFunctorTask<void, Functor> : public QtConcurrent::RunFunctionTask<void>
{
public:
    RunCancelableFunctorTask(Functor func) : m_func(func) { }
    void runFunctor() override
    {
        OperationToken token(this);
        m_func(token);
    }
private:
    Functor m_func;
};

template <class T>
class HasResultType
{
    typedef char Yes;
    typedef void *No;
    template<typename U> static Yes test(int, const typename U::result_type * = 0);
    template<typename U> static No test(double);
public:
    enum { Value = (sizeof(test<T>(0)) == sizeof(Yes)) };
};

class CancelableTaskExecutor
{
public:
    //function<T or void (const OperationToken& token)>
    template <typename Functor>
    static auto run(Functor functor)
        -> typename std::enable_if<!HasResultType<Functor>::Value,
                        QFuture<decltype(functor(std::declval<const OperationToken&>()))>>::type
    {
        typedef decltype(functor(std::declval<const OperationToken&>())) result_type;
        return (new RunCancelableFunctorTask<result_type, Functor>(functor))->start();
    }
};
#endif // FUNCTORTASKS_H
