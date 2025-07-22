#ifndef MODEL_HPP
#define MODEL_HPP

class ModelListener;

class Model
{
public:
    Model();

    void bind(ModelListener* listener)
    {
        modelListener = listener;
    }

    void tick();

    // Model 側でイベントが発生したと仮定した関数
    void triggerSomeEvent();

protected:
    ModelListener* modelListener {nullptr};
};

#endif // MODEL_HPP
