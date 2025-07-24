#ifndef MODEL_HPP
#define MODEL_HPP

#include <cstdint>

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

    // ★ 追加：Presenter間で値を共有
    void        setDesiredValue(uint32_t v) { desiredValue = v; }
    uint32_t    getDesiredValue() const     { return desiredValue; }
    // Model 側でイベントが発生したと仮定した関数
    void triggerSomeEvent();

private:
    // ▼ 追加：ユーザーが入力した値（例：AD5292 用抵抗値）
    uint32_t desiredValue = 0;

protected:
    ModelListener* modelListener {nullptr};
};

#endif // MODEL_HPP
