#include "particle_system/manager/manager.h"

using namespace			particle_system;

						manager::manager(engine::core &engine, computer::core &computer) :
						engine(engine),
						computer(computer),
						renderers(number_of_particles)
{
	initialize_engine();
	initialize_computer();
	
	arguments.position.acquire();
	kernels.xorshift_seed.run();
	kernels.particle_reset.run();
	kernels.initialize_as_sphere.run();
	arguments.position.release();
}

engine::renderer		&manager::receive_particle_renderer()
{
	return (renderers.particle);
}

engine::renderer		&manager::receive_cube_renderer()
{
	return (renderers.cube);
}

void					manager::parse(const string &source)
{
	map.emplace(source);
}

void					manager::initialize_engine()
{
	timer = &engine.generate_timer(1.f / 100.f, &manager::timer_function, this);
	timer->block = true;

	engine.generate_callback(engine::event::type::key_press, &manager::callback_function, this);
	engine.generate_callback(engine::event::type::key_hold, &manager::callback_function, this);
}

void					manager::initialize_computer()
{
//						KERNELS

	kernels.xorshift_seed = computer.generate_kernel();
	kernels.xorshift_seed.add_source("project/resources/OpenCL/xorshift.txt");
	kernels.xorshift_seed.build("xorshift_seed", number_of_particles);

	kernels.particle_reset = computer.generate_kernel();
	kernels.particle_reset.add_source("project/resources/OpenCL/vector.txt");
	kernels.particle_reset.add_source("project/resources/OpenCL/particle_x.txt");
	kernels.particle_reset.build("particle_reset", number_of_particles);

	kernels.particle_update = computer.generate_kernel();
	kernels.particle_update.add_source("project/resources/OpenCL/vector.txt");
	kernels.particle_update.add_source("project/resources/OpenCL/particle_x.txt");
	kernels.particle_update.build("particle_update", number_of_particles);

	kernels.initialize_as_null = computer.generate_kernel();
	kernels.initialize_as_null.add_source("project/resources/OpenCL/xorshift.txt");
	kernels.initialize_as_null.add_source("project/resources/OpenCL/vector.txt");
	kernels.initialize_as_null.add_source("project/resources/OpenCL/initialize_as_x.txt");
	kernels.initialize_as_null.build("initialize_as_null", number_of_particles);

	kernels.initialize_as_cube = computer.generate_kernel();
	kernels.initialize_as_cube.add_source("project/resources/OpenCL/xorshift.txt");
	kernels.initialize_as_cube.add_source("project/resources/OpenCL/vector.txt");
	kernels.initialize_as_cube.add_source("project/resources/OpenCL/initialize_as_x.txt");
	kernels.initialize_as_cube.build("initialize_as_cube", number_of_particles);

	kernels.initialize_as_sphere = computer.generate_kernel();
	kernels.initialize_as_sphere.add_source("project/resources/OpenCL/xorshift.txt");
	kernels.initialize_as_sphere.add_source("project/resources/OpenCL/vector.txt");
	kernels.initialize_as_sphere.add_source("project/resources/OpenCL/initialize_as_x.txt");
	kernels.initialize_as_sphere.build("initialize_as_sphere", number_of_particles);

	kernels.attractor_execute = computer.generate_kernel();
	kernels.attractor_execute.add_source("project/resources/OpenCL/vector.txt");
	kernels.attractor_execute.add_source("project/resources/OpenCL/macros.txt");
	kernels.attractor_execute.add_source("project/resources/OpenCL/object.txt");
	kernels.attractor_execute.add_source("project/resources/OpenCL/attractor_x.txt");
	kernels.attractor_execute.build("attractor_execute", number_of_particles);

	kernels.emitter_start = computer.generate_kernel();
	kernels.emitter_start.add_source("project/resources/OpenCL/xorshift.txt");
	kernels.emitter_start.add_source("project/resources/OpenCL/vector.txt");
	kernels.emitter_start.add_source("project/resources/OpenCL/object.txt");
	kernels.emitter_start.add_source("project/resources/OpenCL/emitter_x.txt");
	kernels.emitter_start.build("emitter_start", 1);

	kernels.emitter_finish = computer.generate_kernel();
	kernels.emitter_finish.add_source("project/resources/OpenCL/xorshift.txt");
	kernels.emitter_finish.add_source("project/resources/OpenCL/vector.txt");
	kernels.emitter_finish.add_source("project/resources/OpenCL/object.txt");
	kernels.emitter_finish.add_source("project/resources/OpenCL/emitter_x.txt");
	kernels.emitter_finish.build("emitter_finish", number_of_particles);

	kernels.emitter_execute = computer.generate_kernel();
	kernels.emitter_execute.add_source("project/resources/OpenCL/xorshift.txt");
	kernels.emitter_execute.add_source("project/resources/OpenCL/vector.txt");
	kernels.emitter_execute.add_source("project/resources/OpenCL/object.txt");
	kernels.emitter_execute.add_source("project/resources/OpenCL/emitter_x.txt");
	kernels.emitter_execute.build("emitter_execute", number_of_particles);

	kernels.consumer_execute = computer.generate_kernel();
	kernels.consumer_execute.add_source("project/resources/OpenCL/vector.txt");
	kernels.consumer_execute.add_source("project/resources/OpenCL/macros.txt");
	kernels.consumer_execute.add_source("project/resources/OpenCL/object.txt");
	kernels.consumer_execute.add_source("project/resources/OpenCL/particle_x.txt");
	kernels.consumer_execute.add_source("project/resources/OpenCL/consumer_x.txt");
	kernels.consumer_execute.build("consumer_execute", number_of_particles);

//						ARGUMENTS INITIALIZATION

	arguments.number_of_particles = kernels.emitter_start.generate_argument<int>();
	arguments.number_of_objects = kernels.emitter_start.generate_argument<int>();
	arguments.object_type = kernels.emitter_start.generate_argument<int>(number_of_objects);
	arguments.object_position = kernels.emitter_start.generate_argument<float, 3>(number_of_objects);
	arguments.is_alive = kernels.emitter_start.generate_argument<char>(number_of_particles);
	arguments.position = kernels.particle_reset.generate_argument(renderers.particle.buffer.receive_attribute(0));
	arguments.velocity = kernels.particle_reset.generate_argument<float, 3>(number_of_particles );
	arguments.acceleration = kernels.particle_reset.generate_argument<float, 3>(number_of_particles);
	arguments.xorshift_state = kernels.xorshift_seed.generate_argument<uint32_t>(number_of_particles);
	arguments.born_by_emitter = kernels.emitter_start.generate_argument<int>(number_of_particles);

//						ARGUMENTS WRITING

	arguments.number_of_particles.write(&number_of_particles);
	arguments.number_of_objects.write(&number_of_objects);

#pragma message "Default scene"
	if (!map)
		parse("maps/demo_a.json");

	object::type		type_data[number_of_objects];
	float				position_data[number_of_objects * 3];

	fill(type_data, type_data + number_of_objects, object::type::empty);
	fill(position_data, position_data + number_of_objects, 0.f);

	for (int i = 0; i < map->size() and i < number_of_objects; i++)
		type_data[i] = map->at(i).read_type();

	for (int i = 0; i < map->size() and i < number_of_objects; i++)
		for (int j = 0; j < 3; j++)
			position_data[i * 3 + j] = map->at(i).read_position()[j];

	arguments.object_type.write(&type_data);
	arguments.object_position.write(&position_data);

//						ARGUMENTS LINKING

	kernels.xorshift_seed.link_argument(arguments.xorshift_state);

	kernels.initialize_as_null.link_argument(arguments.is_alive);
	kernels.initialize_as_null.link_argument(arguments.position);
	kernels.initialize_as_null.link_argument(arguments.born_by_emitter);

	kernels.initialize_as_cube.link_argument(arguments.is_alive);
	kernels.initialize_as_cube.link_argument(arguments.position);
	kernels.initialize_as_cube.link_argument(arguments.xorshift_state);
	kernels.initialize_as_cube.link_argument(arguments.born_by_emitter);

	kernels.initialize_as_sphere.link_argument(arguments.is_alive);
	kernels.initialize_as_sphere.link_argument(arguments.position);
	kernels.initialize_as_sphere.link_argument(arguments.xorshift_state);
	kernels.initialize_as_sphere.link_argument(arguments.born_by_emitter);

	kernels.particle_reset.link_argument(arguments.position);
	kernels.particle_reset.link_argument(arguments.velocity);
	kernels.particle_reset.link_argument(arguments.acceleration);

	kernels.particle_update.link_argument(arguments.position);
	kernels.particle_update.link_argument(arguments.velocity);
	kernels.particle_update.link_argument(arguments.acceleration);

	kernels.attractor_execute.link_argument(arguments.number_of_objects);
	kernels.attractor_execute.link_argument(arguments.object_type);
	kernels.attractor_execute.link_argument(arguments.object_position);
	kernels.attractor_execute.link_argument(arguments.is_alive);
	kernels.attractor_execute.link_argument(arguments.position);
	kernels.attractor_execute.link_argument(arguments.velocity);
	kernels.attractor_execute.link_argument(arguments.acceleration);

	kernels.emitter_start.link_argument(arguments.number_of_particles);
	kernels.emitter_start.link_argument(arguments.number_of_objects);
	kernels.emitter_start.link_argument(arguments.object_type);
	kernels.emitter_start.link_argument(arguments.object_position);
	kernels.emitter_start.link_argument(arguments.is_alive);
	kernels.emitter_start.link_argument(arguments.born_by_emitter);

	kernels.emitter_finish.link_argument(arguments.is_alive);
	kernels.emitter_finish.link_argument(arguments.born_by_emitter);

	kernels.emitter_execute.link_argument(arguments.object_position);
	kernels.emitter_execute.link_argument(arguments.position);
	kernels.emitter_execute.link_argument(arguments.velocity);
	kernels.emitter_execute.link_argument(arguments.acceleration);
	kernels.emitter_execute.link_argument(arguments.born_by_emitter);
	kernels.emitter_execute.link_argument(arguments.xorshift_state);

	kernels.consumer_execute.link_argument(arguments.number_of_objects);
	kernels.consumer_execute.link_argument(arguments.object_type);
	kernels.consumer_execute.link_argument(arguments.object_position);
	kernels.consumer_execute.link_argument(arguments.is_alive);
	kernels.consumer_execute.link_argument(arguments.position);
	kernels.consumer_execute.link_argument(arguments.velocity);
	kernels.consumer_execute.link_argument(arguments.acceleration);
}

void 					manager::timer_function()
{
	arguments.position.acquire();

//						Attractor and repeller
	kernels.attractor_execute.run();

//						Particle
	kernels.particle_update.run();

//						Emitter and consumer
	kernels.emitter_start.run();
	kernels.emitter_execute.run();
	kernels.emitter_finish.run();

	kernels.consumer_execute.run();

	arguments.position.release();
	engine::core::should_render = true;
}

void 					manager::callback_function()
{
	engine::event		&event = engine.receive_event();

	if (event.read_key() == engine::event::key::escape)
		engine.finish();
	else if (event.read_key() == engine::event::key::letter_a)
		renderers.camera.move(engine::axis::x, engine::sign::minus);
	else if (event.read_key() == engine::event::key::letter_d)
		renderers.camera.move(engine::axis::x, engine::sign::plus);
	else if (event.read_key() == engine::event::key::letter_w)
		renderers.camera.move(engine::axis::z, engine::sign::minus);
	else if (event.read_key() == engine::event::key::letter_s)
		renderers.camera.move(engine::axis::z, engine::sign::plus);
	else if (event.read_key() == engine::event::key::letter_q)
		renderers.camera.move(engine::axis::y, engine::sign::plus);
	else if (event.read_key() == engine::event::key::letter_e)
		renderers.camera.move(engine::axis::y, engine::sign::minus);
	else if (event.read_key() == engine::event::key::left)
		renderers.camera.rotate(engine::axis::y, engine::sign::plus);
	else if (event.read_key() == engine::event::key::right)
		renderers.camera.rotate(engine::axis::y, engine::sign::minus);
	else if (event.read_key() == engine::event::key::up)
		renderers.camera.rotate(engine::axis::x, engine::sign::plus);
	else if (event.read_key() == engine::event::key::down)
		renderers.camera.rotate(engine::axis::x, engine::sign::minus);
	else if (event.read_key() == engine::event::key::enter)
		timer->block = not timer->block;
	else
		return ;
	engine::core::should_render = true;
}