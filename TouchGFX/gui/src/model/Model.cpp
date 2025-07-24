#include <gui/model/Model.hpp>
#include <gui/model/ModelListener.hpp>


Model::Model() : modelListener(nullptr)
{
}



void Model::bind(ModelListener* listener)
{
    modelListener = listener;
}

void Model::tick()
{
    // 定期処理など
}

void Model::triggerSomeEvent()
{
    if (modelListener) {
        modelListener->onSomeEvent(); // Presenter へ通知
    }
}

void Model::setLastInputValue(uint32_t v)
{
    lastInputValue = v;
}

uint32_t Model::getLastInputValue() const
{
    return lastInputValue;
}



void Model::setDesiredValue(uint32_t v)
{
    desiredValue = v;
}

uint32_t Model::getDesiredValue() const
{
    return desiredValue;
}



