#include "scriptManager.hpp"

#include <fstream>
#include <sstream>

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/RectangleShape.hpp>

namespace aw
{
	ScriptManager::ScriptManager() :
		m_checkpoint(nullptr)
	{}

	void ScriptManager::update(const sf::Time &frameTime, Player &player, Camera &camera)
	{
		//Update the scriptActions, they got added through the scripts
		for (auto &it : m_scriptActions)
		{
			it.upate(frameTime, player, camera);
		}

		//Remove scriptActions when they finished their work
		m_scriptActions.remove_if( [] (ScriptAction &it) {return it.isFinished(); } );

		//Check for not activated Scripts
		//Some of them will add scriptActions to the list...
		for (auto &it : m_scripts)
		{
			if (!it.used)
			{
				if (player.getVertexPosition(0).x - 15 >= it.xPos * 25.f) //Front vertex
				{
					it.used = true;

					switch (it.type)
					{
					case CHECKPOINT: checkPointAction(player, camera); break;
					case CHANGE_SPEED: changeSpeedAction(it.first, it.second); break;
					case CHANGE_GRAVITY: changeGravityAction(it.first, it.second); break;
					case ROTATE_CAMERA: rotateCamera(it.first, it.second); break;
					case ZOOM: zoomAction(it.first, it.second); break;
					case CAMERA_OFFSET: cameraOffset(it.first, it.second, it.third); break;
					case TILT_X: tiltX(it.first); break;
					case TILT_Y: tiltY(it.first); break;
					default: break;
					}
				}
			}
		}
	}

	void ScriptManager::load(const std::string &path, bool online)
	{
		//Clear the script vector and the checkpoint pointer
		m_scripts.clear();
		m_scriptActions.clear();
		m_checkpoint = nullptr;

		std::fstream file(path.c_str(), std::ios::in);

		if (!file.good())
			return;
		
		std::string line;
		while (std::getline(file, line))
		{
			if (line == "[Scripts]")
			{
				while (std::getline(file, line))
				{
					if (line == "[/Scripts]")
					{
						break;
					}
					
					int type;
					int xPos;
					float first, second, third;

					std::stringstream sstr(line);
					sstr >> type >> xPos >> first >> second >> third;
					
					//If in online-mode dont load checkpoints and speed-up/downs
					if (online)
					{
						if (type == ScriptType::CHANGE_SPEED || type == ScriptType::CHECKPOINT)
						{
							continue;
						}
					}
					m_scripts.push_back(Script(ScriptType(type), xPos, first, second, third));
				}
			}
		}

		file.close();
	}

	void ScriptManager::render(sf::RenderWindow &window)
	{
		int screenStartX = static_cast<int>(window.getView().getCenter().x - (window.getView().getSize().x / 2.f));
		int screenEndX = static_cast<int>(window.getView().getCenter().x + (window.getView().getSize().x / 2.f));

		sf::RectangleShape toDraw; 
		 
		for (auto &it : m_scripts)
		{
			//+16 to prevent the script to disappear too early (position + 10, size +5, +1 because i can xD ) 
			if (it.xPos * 25.f + 16 >= screenStartX && it.xPos * 25.f <= screenEndX)
			{
				//The script is visible
				//update position of toDraw
				toDraw.setPosition(sf::Vector2f((it.xPos * 25.f) + 10.f, window.getView().getCenter().y - (window.getView().getSize().y / 2.f) - 250.f));
				toDraw.setSize(sf::Vector2f(5, window.getView().getSize().x + 500.f));

				//Color depends on the script type
				switch (it.type)
				{
				case ScriptType::CHECKPOINT: toDraw.setFillColor(sf::Color(255, 127, 36)); break;
				case ScriptType::CHANGE_GRAVITY: toDraw.setFillColor(sf::Color(255, 255, 0)); break;
				case ScriptType::CHANGE_SPEED: toDraw.setFillColor(sf::Color(127, 250, 0)); break;
				case ScriptType::TILT_X:;
				case ScriptType::TILT_Y: toDraw.setFillColor(sf::Color(127, 127, 127)); break;
				default: toDraw.setFillColor(sf::Color(0, 0, 205)); break;//Camera scripts
				}

				//Draw it
				window.draw(toDraw);
				if (m_checkpoint)
				{
					m_checkpoint->savedPlayer.render(window);
				}
			}
		}
	}


	Checkpoint *ScriptManager::getLastCheckPoint()
	{
		if (m_checkpoint)
		{
			return &*m_checkpoint;
		}

		return nullptr;
	}

	void ScriptManager::deleteScripts()
	{
		m_scripts.clear();
		m_scriptActions.clear();
	}

	void ScriptManager::resetScriptStates(bool resetCheckpoints)
	{
		if (resetCheckpoints)
		{
			m_checkpoint = nullptr;
		}

		//Reset action scripts too
		m_scriptActions.clear();

		if (m_checkpoint)
			m_scriptActions = m_checkpoint->savedScriptActions;

		for (auto &it : m_scripts)
		{
			if (resetCheckpoints || !m_checkpoint)
			{
				it.used = false;
			}
			else
			{
				if (m_checkpoint->savedPlayer.getVertexPosition(0).x - 15 < it.xPos * 25.f)
				{
					if (it.type != ScriptType::CHECKPOINT)
						it.used = false;
				}
			}
		}
	}

	////////ACTION FUNCTIONS//////////////////////////////////////////////////////////

	void ScriptManager::checkPointAction(Player &player, Camera &camera)
	{
		m_checkpoint = std::unique_ptr<Checkpoint>(new Checkpoint());
		m_checkpoint->savedPlayer = player;
		m_checkpoint->savedCamera = camera;
		m_checkpoint->savedScriptActions = m_scriptActions;
	}

	void ScriptManager::changeSpeedAction(float value, float duration)
	{
		m_scriptActions.push_back(ScriptAction(ScriptType::CHANGE_SPEED, sf::seconds(duration), value));
	}
	void ScriptManager::changeGravityAction(float value, float duration)
	{
		m_scriptActions.push_back(ScriptAction(ScriptType::CHANGE_GRAVITY, sf::seconds(duration), value));
	}
	void ScriptManager::rotateCamera(float value, float duration)
	{
		m_scriptActions.push_back(ScriptAction(ScriptType::ROTATE_CAMERA, sf::seconds(duration), value));
	}
	void ScriptManager::zoomAction(float value, float duration)
	{
		m_scriptActions.push_back(ScriptAction(ScriptType::ZOOM, sf::seconds(duration), value));
	}
	void ScriptManager::cameraOffset(float xValue, float yValue, float duration)
	{
		m_scriptActions.push_back(ScriptAction(ScriptType::CAMERA_OFFSET, sf::seconds(duration), xValue, yValue));
	}
	void ScriptManager::tiltX(float duration)
	{
		m_scriptActions.push_back(ScriptAction(ScriptType::TILT_X, sf::seconds(duration)));
	}
	void ScriptManager::tiltY(float duration)
	{
		m_scriptActions.push_back(ScriptAction(ScriptType::TILT_Y, sf::seconds(duration)));
	}
}