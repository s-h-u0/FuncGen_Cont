#ifndef MODELLISTENER_HPP
#define MODELLISTENER_HPP

class Model;

class ModelListener
{
public:
    ModelListener() : model(nullptr) {}
    virtual ~ModelListener() {}

    void bind(Model* m) { model = m; }

    // Model から Presenter へ通知する仮想関数
    virtual void onSomeEvent() {}

protected:
    Model* model;
};

#endif // MODELLISTENER_HPP
