#include "GameScene.h"
#include "KamataEngine.h"

#include "json.hpp"
#include <cassert>
#include <fstream>
#include <vector>

#include <DirectXMath.h>

using namespace KamataEngine;

GameScene::~GameScene() {
	// オブジェクトのメモリ解放
	for (WorldTransform* obj : objects) {
		delete obj;
	}
	delete levelData;
}

void GameScene::SceneJson() {
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
	// モデルの読み込み
	for (auto& objectData : levelData->objects) {
		Model* model = nullptr;
		auto it = models.find(objectData.file_name);
		if (it == models.end()) {
			model = Model::CreateFromOBJ(objectData.file_name);
			models[objectData.file_name] = model;
		}
	}

	// オブジェクト生成
	for (auto& objectData : levelData->objects) {
		if (objectData.name == "player") {
			playerModel_ = models[objectData.file_name];
			playerTransform_.translation_ = objectData.transform.translation;
			playerTransform_.rotation_ = objectData.transform.rotation;
			playerTransform_.scale_ = objectData.transform.scaling;
			playerTransform_.Initialize();
		} else if (objectData.name == "plane") {
			planeModel_ = models[objectData.file_name];
			planeTransform_.translation_ = objectData.transform.translation;
			planeTransform_.rotation_ = objectData.transform.rotation;
			planeTransform_.scale_ = objectData.transform.scaling;
			planeTransform_.Initialize();
		} else if (objectData.name.find("enemy") != std::string::npos) {
			if (!enemyModel_) {
				enemyModel_ = Model::CreateFromOBJ(objectData.file_name);
			}
			WorldTransform* enemyTransform = new WorldTransform;
			enemyTransform->translation_ = objectData.transform.translation;
			enemyTransform->rotation_ = objectData.transform.rotation;
			enemyTransform->scale_ = objectData.transform.scaling;
			enemyTransform->Initialize();
			enemyTransforms_.push_back(enemyTransform);
		} else {
			WorldTransform* newObject = new WorldTransform;
			newObject->translation_ = objectData.transform.translation;
			newObject->rotation_ = objectData.transform.rotation;
			newObject->scale_ = objectData.transform.scaling;
			newObject->Initialize();
			objects.push_back(newObject);
		}
	}
}

void GameScene::Initialize() {
	dxCommon_ = DirectXCommon::GetInstance();
	input_ = Input::GetInstance();
	audio_ = Audio::GetInstance();
	camera_.Initialize();

	playerModel_ = Model::CreateFromOBJ("player");
	planeModel_ = Model::CreateFromOBJ("plane");
	enemyModel_ = Model::CreateFromOBJ("enemy");
	
	// jsonファイルの関数
	SceneJson();

}

void GameScene::Update() {
	for (WorldTransform* object : objects) {
		object->TransferMatrix();
	}
	playerTransform_.TransferMatrix();
	planeTransform_.TransferMatrix();

}

void GameScene::Draw() {
	ID3D12GraphicsCommandList* commandList = dxCommon_->GetCommandList();

	// 背景
	Sprite::PreDraw(commandList);
	Sprite::PostDraw();
	dxCommon_->ClearDepthBuffer();

	// 3Dオブジェクト
	Model::PreDraw(commandList);

	//player
	playerModel_->Draw(playerTransform_, camera_);
	// Plane
	planeModel_->Draw(planeTransform_, camera_);
	//enemy
	for (WorldTransform* enemy : enemyTransforms_) {
		enemyModel_->Draw(*enemy, camera_);
	}

	Model::PostDraw();

	// 前景
	Sprite::PreDraw(commandList);
	Sprite::PostDraw();
}
