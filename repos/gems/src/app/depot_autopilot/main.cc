/*
 * \brief  Tool for turning a subsystem blueprint into an init configuration
 * \author Norman Feske
 * \date   2017-07-07
 */

/*
 * Copyright (C) 2017 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

/* Genode includes */
#include <util/retry.h>
#include <base/component.h>
#include <base/attached_rom_dataspace.h>
#include <base/heap.h>
#include <os/reporter.h>
#include <root/component.h>

/* local includes */
#include "children.h"

namespace Depot_deploy {

	class  Log_session_component;
	class  Log_root;
	struct Main;
}


class Depot_deploy::Log_session_component : public Rpc_object<Log_session>
{
	private:

		Session_label const  _child_label;
		Child               &_child;

	public:

		Log_session_component(Session_label const &child_label,
		                      Child               &child)
		:
			_child_label(child_label),
			_child(child)
		{ }

		size_t write(String const &line) override
		{
			if (_child.finished()) {
				return 0; }

			Log_event::Line line_labeled{ "[", _child_label.string(), "] ", line.string() };
			_child.log_session_write(line_labeled);
			return strlen(line.string());
		}
};


class Depot_deploy::Log_root : public Root_component<Log_session_component>
{
	public:

		Children            &_children;
		Session_label const &_children_label_prefix;

		Log_root(Entrypoint          &ep,
		         Allocator           &md_alloc,
		         Children            &children,
		         Session_label const &children_label_prefix)
		:
			Root_component         { ep, md_alloc },
			_children              { children },
			_children_label_prefix { children_label_prefix }
		{ }

		Log_session_component *_create_session(const char *args, Affinity const &)
		{
			using Name_delimiter = String<5>;

			Session_label const label            { label_from_args(args) };
			size_t        const label_prefix_len { strlen(_children_label_prefix.string()) };
			Session_label const label_prefix     { Cstring(label.string(), label_prefix_len) };

			if (label_prefix != _children_label_prefix) {
				warning("LOG session label does not have children label-prefix");
				throw Service_denied();
			}
			char    const *const name_base  { label.string() + label_prefix_len };
			Name_delimiter const name_delim { " -> " };

			size_t name_len { 0 };
			for (char const *str = name_base; str[name_len] &&
			     name_delim != Name_delimiter(str + name_len); name_len++);

			Child::Name       name       { Cstring(name_base, name_len) };
			char const *const label_base { name_base + name_len };

			try {
				return new (md_alloc())
					Log_session_component(Session_label("init", label_base),
					                      _children.find_by_name(name));
			}
			catch (Children::No_match) {
				warning("Cannot find child by LOG session label");
				throw Service_denied();
			}
		}
};


struct Depot_deploy::Main
{
	typedef String<128> Name;

	Env                           &_env;
	Attached_rom_dataspace         _config                { _env, "config" };
	Attached_rom_dataspace         _blueprint             { _env, "blueprint" };
	Expanding_reporter             _query_reporter        { _env, "query" , "query"};
	Expanding_reporter             _init_config_reporter  { _env, "config", "init.config"};
	Heap                           _heap                  { _env.ram(), _env.rm() };
	Reconstructible<Session_label> _children_label_prefix { };
	Timer::Connection              _timer                 { _env };
	Signal_handler<Main>           _config_handler        { _env.ep(), *this, &Main::_handle_config };
	Children                       _children              { _heap, _timer, _config_handler };
	Log_root                       _log_root              { _env.ep(), _heap, _children, *_children_label_prefix };

	void _handle_config()
	{
		_config.update();
		_blueprint.update();

		Xml_node const config = _config.xml();

		_children_label_prefix.construct(config.attribute_value("children_label_prefix", String<160>()));
		_children.apply_config(config);
		_children.apply_blueprint(_blueprint.xml());

		/* determine CPU architecture of deployment */
		typedef String<16> Arch;
		Arch const arch = config.attribute_value("arch", Arch());
		if (!arch.valid())
			warning("config lacks 'arch' attribute");

		/* generate init config containing all configured start nodes */
		bool finished;
		_init_config_reporter.generate([&] (Xml_generator &xml) {
			Xml_node static_config = config.sub_node("static");
			xml.append(static_config.content_base(), static_config.content_size());
			Child::Depot_rom_server const parent { };
			finished = _children.gen_start_nodes(xml, config.sub_node("common_routes"),
			                                     parent, parent);
		});
		if (finished) {

			unsigned long previous_time_sec { 0UL };
			if (config.has_sub_node("previous-results")) {
				previous_time_sec += config.sub_node("previous-results").attribute_value("time_sec", 0UL);
			}
			unsigned long const time_us  { _timer.curr_time().trunc_to_plain_us().value };
			unsigned long       time_ms  { time_us / 1000UL };
			unsigned long const time_sec { time_ms / 1000UL };
			time_ms = time_ms - time_sec * 1000UL;

			log("\n--- Finished after ", time_sec + previous_time_sec, ".", time_ms < 10 ? "00" : time_ms < 100 ? "0" : "", time_ms, " sec ---\n");
			if (config.has_sub_node("previous-results")) {
				Xml_node const previous_results = config.sub_node("previous-results");
				if (previous_results.content_size()) {
					log(Cstring(previous_results.content_base(), previous_results.content_size()));
				}
			}
			int result = _children.conclusion();
			log("");
			_env.parent().exit(result);

		}

		/* update query for blueprints of all unconfigured start nodes */
		if (arch.valid()) {
			_query_reporter.generate([&] (Xml_generator &xml) {
				xml.attribute("arch", arch);
				_children.gen_queries(xml);
			});
		}
	}

	Main(Env &env) : _env(env)
	{
		_config   .sigh(_config_handler);
		_blueprint.sigh(_config_handler);

		_handle_config();
		_env.parent().announce(_env.ep().manage(_log_root));
	}
};


void Component::construct(Genode::Env &env) { static Depot_deploy::Main main(env); }
