#include "scripts_controller.hpp"

#include <nscapi/nscapi_protobuf.hpp>

#include <str/xtos.hpp>

#include <json_spirit.h>

#include <boost/algorithm/string.hpp>
#include <boost/foreach.hpp>
#include <boost/regex.hpp>
#include <boost/filesystem/path.hpp>

#include <fstream>
#include <iostream>

scripts_controller::scripts_controller(boost::shared_ptr<session_manager_interface> session, nscapi::core_wrapper* core, unsigned int plugin_id)
  : session(session)
  , core(core)
  , plugin_id(plugin_id)
  , RegexpController("/api/v1/scripts")
{
	addRoute("GET", "/?$", this, &scripts_controller::get_runtimes);
	addRoute("GET", "/([^/]+)/?$", this, &scripts_controller::get_scripts);
	addRoute("GET", "/([^/]+)/(.+)/?$", this, &scripts_controller::get_script);
	addRoute("PUT", "/([^/]*)/(.+)/?$", this, &scripts_controller::add_script);
	addRoute("DELETE", "/([^/]*)/(.+)/?$", this, &scripts_controller::delete_script);
}

void scripts_controller::get_runtimes(Mongoose::Request &request, boost::smatch &what, Mongoose::StreamResponse &response) {
  if (!session->is_loggedin(request, response))
    return;

  Plugin::RegistryRequestMessage rrm;
  Plugin::RegistryRequestMessage::Request *payload = rrm.add_payload();
  payload->mutable_inventory()->set_fetch_all(false);
  payload->mutable_inventory()->add_type(Plugin::Registry_ItemType_MODULE);
  std::string str_response;
  core->registry_query(rrm.SerializeAsString(), str_response);

  Plugin::RegistryResponseMessage pb_response;
  pb_response.ParseFromString(str_response);
  json_spirit::Array root;

  BOOST_FOREACH(const Plugin::RegistryResponseMessage::Response r, pb_response.payload()) {
	  BOOST_FOREACH(const Plugin::RegistryResponseMessage::Response::Inventory i, r.inventory()) {
		  if (i.name() == "CheckPython" 
			  || i.name() == "CheckExternalScripts" 
			  || i.name() == "LUAScript") {
			  json_spirit::Object node;
			  if (i.name() == "CheckPython") {
				  node["name"] = "py";
			  } else if (i.name() == "CheckExternalScripts") {
				  node["name"] = "ext";
			  } else if (i.name() == "LUAScript") {
				  node["name"] = "lua";
			  } else {
				  node["name"] = i.name();
			  }
			  node["module"] = i.name();
			  node["title"] = i.info().title();

			  root.push_back(node);
		  }
	  }
  }
  response.append(json_spirit::write(root));
}

void scripts_controller::get_scripts(Mongoose::Request &request, boost::smatch &what, Mongoose::StreamResponse &response) {
	if (!session->is_loggedin(request, response))
		return;

	if (what.size() != 2) {
		response.setCode(HTTP_NOT_FOUND);
		response.append("Script runtime not found");
	}
	std::string runtime = what.str(1);
	if (runtime == "ext") {
		runtime = "ExternalScripts";
	}

	std::string fetch_all = request.get("all", "false");

	Plugin::ExecuteRequestMessage rm;
	Plugin::ExecuteRequestMessage::Request *payload = rm.add_payload();

	payload->set_command("list");
	payload->add_arguments("--json");
	if (fetch_all != "true") {
		payload->add_arguments("--query");
	}

	std::string pb_response;
	core->exec_command(runtime, rm.SerializeAsString(), pb_response);
	Plugin::ExecuteResponseMessage resp;
	resp.ParseFromString(pb_response);
	if (resp.payload_size() != 1) {
		response.setCode(HTTP_NOT_FOUND);
		response.append("Invalid request, perhaps the script runtime is not available?");
		return;
	}
	
	response.append(resp.payload(0).message());
}



void scripts_controller::get_script(Mongoose::Request &request, boost::smatch &what, Mongoose::StreamResponse &response) {
	if (!session->is_loggedin(request, response))
		return;

	if (what.size() != 3) {
		response.setCode(HTTP_NOT_FOUND);
		response.append("Script runtime not found");
	}
	std::string runtime = what.str(1);
	std::string script = what.str(2);
	if (runtime == "ext") {
		runtime = "ExternalScripts";
	}

	Plugin::ExecuteRequestMessage rm;
	Plugin::ExecuteRequestMessage::Request *payload = rm.add_payload();

	payload->set_command("show");
	payload->add_arguments("--script");
	payload->add_arguments(script);

	std::string pb_response;
	core->exec_command(runtime, rm.SerializeAsString(), pb_response);
	Plugin::ExecuteResponseMessage resp;
	resp.ParseFromString(pb_response);
	if (resp.payload_size() != 1) {
		response.setCode(HTTP_NOT_FOUND);
		response.append("Invalid request, perhaps the script runtime is not available?");
		return;
	}

	response.append(resp.payload(0).message());
}



void scripts_controller::add_script(Mongoose::Request &request, boost::smatch &what, Mongoose::StreamResponse &response) {
	if (!session->is_loggedin(request, response))
		return;

	if (what.size() != 3) {
		response.setCode(HTTP_NOT_FOUND);
		response.append("Script runtime not found");
	}
	std::string runtime = what.str(1);
	if (runtime == "ext") {
		runtime = "ExternalScripts";
	}
	std::string script = what.str(2);

	boost::filesystem::path name = script;
	boost::filesystem::path file = core->expand_path("${temp}/" + name.filename().string());
	std::ofstream ofs(file.string(), std::ios::binary);
	ofs << request.getData();
	ofs.close();

	Plugin::ExecuteRequestMessage rm;
	Plugin::ExecuteRequestMessage::Request *payload = rm.add_payload();

	payload->set_command("add");
	payload->add_arguments("--script");
#ifdef WIN32
	boost::algorithm::replace_all(script, "/", "\\");
#endif
	payload->add_arguments(script);
	payload->add_arguments("--import");
	payload->add_arguments(file.string());
	payload->add_arguments("--replace");

	std::string pb_response;
	core->exec_command(runtime, rm.SerializeAsString(), pb_response);
	Plugin::ExecuteResponseMessage resp;
	resp.ParseFromString(pb_response);
	if (resp.payload_size() != 1) {
		response.setCode(HTTP_NOT_FOUND);
		response.append("Invalid request, perhaps the script runtime is not available?");
		return;
	}

	response.append(resp.payload(0).message());

}


void scripts_controller::delete_script(Mongoose::Request &request, boost::smatch &what, Mongoose::StreamResponse &response) {
	if (!session->is_loggedin(request, response))
		return;

	response.setCode(HTTP_NOT_FOUND);
	response.append("Not implemented yet");
}

