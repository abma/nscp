#pragma once

#include <utils.h>
#include <strEx.h>

#include <collectd/collectd_packet.hpp>

#include <collectd/client/collectd_client_protocol.hpp>
#include <socket/client.hpp>

#include <nscapi/nscapi_settings_helper.hpp>
#include <nscapi/nscapi_protobuf_functions.hpp>
#include <nscapi/nscapi_core_helper.hpp>

#include <boost/make_shared.hpp>

#include "collectd_client.hpp"

namespace collectd_handler {
	namespace sh = nscapi::settings_helper;

	struct collectd_target_object : public nscapi::targets::target_object {
		typedef nscapi::targets::target_object parent;

		collectd_target_object(std::string alias, std::string path) : parent(alias, path) {
			set_property_string("port", "25826");
			set_property_string("host", "239.192.74.66");
		}
		collectd_target_object(const nscapi::settings_objects::object_instance other, std::string alias, std::string path) : parent(other, alias, path) {}

		virtual void read(boost::shared_ptr<nscapi::settings_proxy> proxy, bool oneliner, bool is_sample) {
			parent::read(proxy, oneliner, is_sample);

			nscapi::settings_helper::settings_registry settings(proxy);

			nscapi::settings_helper::path_extension root_path = settings.path(get_path());
			if (is_sample)
				root_path.set_sample();

			//add_ssl_keys(root_path);

			settings.register_all();
			settings.notify();
		}
	};

	struct options_reader_impl : public client::options_reader_interface {
		virtual nscapi::settings_objects::object_instance create(std::string alias, std::string path) {
			return boost::make_shared<collectd_target_object>(alias, path);
		}
		virtual nscapi::settings_objects::object_instance clone(nscapi::settings_objects::object_instance parent, const std::string alias, const std::string path) {
			return boost::make_shared<collectd_target_object>(parent, alias, path);
		}

		void process(boost::program_options::options_description &desc, client::destination_container &source, client::destination_container &data) {
			//add_ssl_options(desc, data);

			desc.add_options()

				("payload-length,l", po::value<unsigned int>()->notifier(boost::bind(&client::destination_container::set_int_data, &data, "payload length", _1)),
					"Length of payload (has to be same as on the server)")

				("buffer-length", po::value<unsigned int>()->notifier(boost::bind(&client::destination_container::set_int_data, &data, "payload length", _1)),
					"Length of payload to/from the NRPE agent. This is a hard specific value so you have to \"configure\" (read recompile) your NRPE agent to use the same value for it to work.")

				("password", po::value<std::string>()->notifier(boost::bind(&client::destination_container::set_string_data, &data, "password", _1)),
					"Password")

				("time-offset", po::value<std::string>()->notifier(boost::bind(&client::destination_container::set_string_data, &data, "time offset", _1)),
					"")
				;
		}
	};
}