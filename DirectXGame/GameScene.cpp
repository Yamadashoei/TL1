#include "GameScene.h"
#include "KamataEngine.h"

#include "json.hpp"
#include <cassert>
#include <fstream>
#include <vector>

using namespace KamataEngine;

GameScene::~GameScene() {}

void GameScene::Initialize() {
	dxCommon_ = DirectXCommon::GetInstance();
	input_ = Input::GetInstance();
	audio_ = Audio::GetInstance();
	camera_.Initialize();

	// JSON読み込み
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

	// レベルデータを構造体に格納
	levelData = new LevelData();
	// nameを文字列として認識
	levelData->name = deserialized["name"].get<std::string>();
	// 正しいレベルデータファイルかチェック
	assert(levelData->name == "scene");

	for (nlohmann::json& object : deserialized["objects"]) {
		assert(object.contains("type"));
		if (object["type"].get<std::string>() == "MESH") {

			// 1個分の要素の準備
			levelData->objects.emplace_back(ObjectData{});
			ObjectData& objectData = levelData->objects.back();
			objectData.type = object["type"].get<std::string>();
			objectData.name = object["name"].get<std::string>();
			// transformのパラメーター読み込み
			nlohmann::json& transform = object["transform"];
			objectData.transform.translation.x = (float)transform["translation"][0];
			objectData.transform.translation.y = (float)transform["translation"][1];
			objectData.transform.translation.z = (float)transform["translation"][2];

			objectData.transform.rotation.x = -(float)transform["rotation"][0];
			objectData.transform.rotation.y = -(float)transform["rotation"][2];
			objectData.transform.rotation.z = (float)transform["rotation"][1];

			objectData.transform.scaling.x = (float)transform["scaling"][0];
			objectData.transform.scaling.y = (float)transform["scaling"][2];
			objectData.transform.scaling.z = (float)transform["scaling"][1];

			if (object.contains("file_name")) {
				objectData.file_name = object["file_name"].get<std::string>();
			}
		}
	}

	for (auto& objectData : levelData->objects) {
		Model* model = nullptr;
		auto it = models.find(objectData.file_name);
		if (it == models.end()) {
			model = Model::CreateFromOBJ(objectData.file_name);
			models[objectData.file_name] = model;
		}
	}

	for (auto& objectData : levelData->objects) {
		WorldTransform* newObject = new WorldTransform;
		newObject->translation_ = objectData.transform.translation;
		newObject->rotation_ = objectData.transform.rotation;
		newObject->scale_ = objectData.transform.scaling;
		newObject->Initialize();
		objects.push_back(newObject);
	}

}

void GameScene::Update() {
	for (WorldTransform* object : objects) {
		object->TransferMatrix();
	}
}

void GameScene::Draw() {
	ID3D12GraphicsCommandList* commandList = dxCommon_->GetCommandList();

	// 背景
	Sprite::PreDraw(commandList);
	Sprite::PostDraw();
	dxCommon_->ClearDepthBuffer();

	// 3Dオブジェクト
	Model::PreDraw(commandList);

	int i = 0;
	for (auto& objectData : levelData->objects) {
		Model* model = nullptr;
		auto it = models.find(objectData.file_name);
		if (it != models.end()) {
			model = it->second;
		}
		if (model) {
			model->Draw(*objects[i], camera_);
		}
		i++;
	}

	Model::PostDraw();

	// 前景
	Sprite::PreDraw(commandList);
	Sprite::PostDraw();
}
