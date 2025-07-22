#include <gui/model/Model.hpp>
#include <gui/model/ModelListener.hpp>

Model::Model() : modelListener(nullptr)
{
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
