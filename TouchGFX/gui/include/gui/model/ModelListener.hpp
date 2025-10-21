#ifndef MODELLISTENER_HPP
#define MODELLISTENER_HPP

/**
 * @file   ModelListener.hpp
 * @brief  Model -> Presenter の通知インタフェース
 * @details
 *  - Presenter は本クラスを継承し、必要なイベントをオーバーライドして受け取る
 */
class Model;

class ModelListener
{
public:
    /** @brief 既定コンストラクタ（未バインド） */
    ModelListener() : model(nullptr) {}
    virtual ~ModelListener() {}

    /**
     * @brief  Model からの逆参照をセット
     * @param  m 通知元の Model
     * @note   Presenter 側で一度だけ呼ばれる想定
     */
    void bind(Model* m) { model = m; }

    /**
     * @brief  サンプル用イベント（必要に応じて Presenter でオーバーライド）
     * @note   追加イベントは本クラスに仮想関数を増やして拡張する
     */
    virtual void onSomeEvent() {}

    // RS-485ブリッジから上がってくる“1行メッセージ”の汎用フック
    // 既定では何もしない（後方互換）
    virtual void onRemoteLine(const char* /*line*/) {}

protected:
    /** @brief 参照元 Model（必要に応じて参照に利用） */
    Model* model;
};

#endif // MODELLISTENER_HPP
