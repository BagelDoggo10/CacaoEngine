#pragma once

#include "glm/glm.hpp"

#include "Events/EventSystem.h"

#include "Utilities/Utilities.h"

#include "Graphics/Skybox.h"

namespace CitrusEngine {
	//Base camera type
	class Camera {
	public:
		Camera()
			: clearColor(1) {
			resizeConsumer = new EventConsumer(BIND_MEMBER_FUNC(Camera::ResizeProjectionMatrix));
			EventManager::GetInstance()->SubscribeConsumer("WindowResize", resizeConsumer);
		}

		~Camera() {
			EventManager::GetInstance()->UnsubscribeConsumer("WindowResize", resizeConsumer);
			delete resizeConsumer;
		}

		enum class ClearMode {
			Color, Skybox
		};

		//Get and set position
		virtual glm::vec3 GetPosition() = 0;
		virtual void SetPosition(glm::vec3 pos) = 0;

		//Get and set rotation
		virtual glm::vec3 GetRotation() = 0;
		virtual void SetRotation(glm::vec3 rot) = 0;

		//Get projection matrix (transforms vertices according to how the camera sees them)
		virtual glm::mat4 GetProjectionMatrix() = 0;
		//Get view matrix (transforms vertices according to where the camera is looking)
		virtual glm::mat4 GetViewMatrix() = 0;
		//Get view-projection (combination of view and projection matrix)
		virtual glm::mat4 GetViewProjectionMatrix() = 0;

		//Sets clear color (takes 8-bit unsigned integer vector (0-255 for red, green, and blue))
        void SetClearColor(glm::uvec3 color) { clearColor = { (float)color.r / 256, (float)color.g / 256, (float)color.b / 256, 1.0f }; }
		//Sets active skybox
		void SetSkybox(Skybox* skybox) { clearSkybox = skybox; }
        //Clears color and depth buffers
        void Clear();

		ClearMode clearMode;

		//Update camera projection matrix for new aspect ratio
		virtual void ResizeProjectionMatrix(Event& e) = 0;
	private:
		EventConsumer* resizeConsumer;

		glm::vec4 clearColor;
		Skybox* clearSkybox;
	};
}