#include "engine/engine.h"
#include "computer/computer.h"
#include "particle_system/particle_system.h"
#include "external/CLI11.hpp"

class							final
{
public :
								final() = default;
								~final() = default;

	bool						process_options(int argc, char **argv)
	{
		std::string				demo_string;
		std::string				map_string;
		std::string				resolution_string;

		auto					app = CLI::App("");

		app.set_help_flag();

		app.add_option("--demo", demo_string);
		app.add_option("--map", map_string);
		app.add_option("--resolution", resolution_string);

		auto					flag_help = app.add_flag("--help");
		auto					flag_fps = app.add_flag("--fps");

		try
		{
			app.parse(argc, argv);
		}
		catch (const CLI::ParseError &e)
		{
			throw (common::exception("Final : Invalid flags, use --help"));
		}

		if (*flag_help)
		{
			std::cout
				<< std::endl
				<< "Particle system " << std::endl
				<< std::endl
				<< "Flags : " << std::endl
				<< "--fps : Print FPS" << std::endl
				<< std::endl
				<< "Keys : " << std::endl
				<< std::endl;
			return (false);
		}

		engine::core::print_fps = *flag_fps ? true : false;

		if (not demo_string.empty())
			map = "maps/demo_" + demo_string + ".json";
		else
			map = map_string;

		if (not resolution_string.empty())
		{
			std::stringstream	stream(resolution_string);

			auto				extract_int = [&stream]()
			{
				int value;

				if ((stream >> value).fail())
					throw (common::exception("Final : Bad resolution value"));
				return (value);
			};

			resolution.emplace();
			resolution->x = extract_int();
			if (stream.peek() != ',')
				throw (common::exception("Final : Bad resolution pattern"));
			stream.ignore();
			resolution->y = extract_int();
		}

		return (true);
	}

	void						setup_static()
	{
		engine::core::initial_window_size =
			resolution ? *resolution : engine::ivec2(1280, 720);
		engine::core::window_name = "Particle System";
		engine::core::use_multisampling = true;
		engine::core::use_blending = true;
		engine::core::use_depth_test = true;
		engine::core::number_of_samples = 16;
		engine::core::point_size = 1;
		engine::core::should_render = true;

		engine::camera::start_position = engine::vec3(0.f, 0.f, 2500.f);
		engine::camera::movement_speed = 10.f;
		engine::camera::rotation_speed = .3f;
		engine::camera::near_plane = 10.f;
		engine::camera::far_plane = 20000.f;

		computer::core::use_OpenGL = true;
	}

	void						setup_dynamic()
	{
		engine.emplace();
		computer.emplace();
		system.emplace(*engine, *computer);

		engine->attach_renderer(system->receive_renderer());
		system->build(map);
	}

	void						run()
	{
		engine->start();
	}

private :

	using						engine_type = std::optional<engine::core>;
	using						computer_type = std::optional<computer::core>;
	using						system_type = std::optional<particle_system::manager>;

	std::string					map;
	std::optional<glm::ivec2>	resolution;

	engine_type					engine;
	computer_type				computer;
	system_type					system;
};

int								main(int argc, char **argv)
{
	try
	{
		final					final;

		if (!final.process_options(argc, argv))
			return (0);

		final.setup_static();
		final.setup_dynamic();
		final.run();
		return (0);
	}
	catch (std::exception &exception)
	{
		std::cerr << "Particle System : Exiting with uncaught exception" << std::endl;
		std::cerr << exception.what() << std::endl;
		return (1);
	}
}
