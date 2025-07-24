#include "GameScene.h"
#include "KamataEngine.h"

#include "json.hpp"
#include <cassert>
#include <fstream>
#include <string>
#include <vector>

using namespace KamataEngine;
using json = nlohmann::json;

GameScene::~GameScene() {}

void GameScene::Initialize() {
	dxCommon_ = DirectXCommon::GetInstance();
	input_ = Input::GetInstance();
	audio_ = Audio::GetInstance();

	// カメラの初期化
	camera_.Initialize();

	// オブジェクト1個分のデータ
	struct ObjectData {
		std::string type;
		std::string name;

		// transform
		struct Transform {
			Vector3 translation;
			Vector3 rotation;
			Vector3 scaling;
		};
		Transform transform;
		std::string file_name;
	};

	//
	struct LevelData {
		// name
		std::string name;
		// objects
		std::vector<ObjectData> objects;
	};

	// JSONファイルを読み込む
	const std::string fullpath = std::string("Resources/levels/") + "scene.json";

	// JSONファイルを開く
	std::ifstream file;

	// ファイルを開く
	file.open(fullpath);
	// ファイルオープン失敗のチェック
	if (file.fail()) {
		assert(0);
	}

	nlohmann::json deserialized;

	// ファイルからJSONを読み込む
	file >> deserialized;

	// 正しいレベルデータファイルかチェック
	assert(deserialized.is_object());
	assert(deserialized.contains("name"));
	assert(deserialized["name"].is_string());

	// ------------------------------------------------------------
	// レベルデータを構造体に格納していく
	// ------------------------------------------------------------
	LevelData* levelData = new LevelData();

	// "name" を文字列として取得
	levelData->name = deserialized["name"].get<std::string>();
	assert(levelData->name == "scene"); // それは "scene" か?

	// "objects" の全オブジェクトを走査
	for (nlohmann::json& object : deserialized["objects"]) {

		// オブジェクト 1 つ分の妥当性のチェック
		assert(object.contains("type")); // "type" が含まれているか

		if (object["type"].get<std::string>() == "MESH") {

			// 1 個分の要素の準備
			levelData->objects.emplace_back(ObjectData{});
			ObjectData& objectData = levelData->objects.back(); // 追加要素の参照を用意して可読性も良くなる

			objectData.type = object["type"].get<std::string>(); // "type"
			objectData.name = object["name"].get<std::string>(); // "name"

			// トランスフォームのパラメータ読み込み
			nlohmann::json& transform = object["transform"];
			// 平行移動 "translation"
			objectData.transform.translation.x = (float)transform["translation"][0];
			objectData.transform.translation.y = (float)transform["translation"][1];
			objectData.transform.translation.z = (float)transform["translation"][2];
			// 回転 "rotation"
			objectData.transform.rotation.x = -(float)transform["rotation"][0];
			objectData.transform.rotation.y = (float)transform["rotation"][2];
			objectData.transform.rotation.z = (float)transform["rotation"][1];
			// 拡大縮小 "scaling"
			objectData.transform.scaling.x = (float)transform["scaling"][0];
			objectData.transform.scaling.y = (float)transform["scaling"][2];
			objectData.transform.scaling.z = (float)transform["scaling"][1];

			// "file_name"
			if (object.contains("file_name")) {
				objectData.file_name = object["file_name"].get<std::string>();
			}
		}
	}
}

void GameScene::Update() {}

void GameScene::Draw() {
	ID3D12GraphicsCommandList* commandList = dxCommon_->GetCommandList();

#pragma region 背景スプライト描画
	Sprite::PreDraw(commandList);
	Sprite::PostDraw();
	dxCommon_->ClearDepthBuffer();
#pragma endregion

#pragma region 3Dオブジェクト描画
	Model::PreDraw(commandList);

	Model::PostDraw();
#pragma endregion

#pragma region 前景スプライト描画
	Sprite::PreDraw(commandList);
	Sprite::PostDraw();
#pragma endregion
}
