//
// This is just experimentation code to figure out the config API in 0.4x
//
// Compile with:
//  g++ -Wall -o oextract oextract.cc $(pkg-config --cflags --libs libopensync1)
//

#include <iostream>
#include <stdexcept>
#include <opensync/opensync.h>
#include <opensync/opensync-plugin.h>
#include <opensync/opensync-group.h>
using namespace std;

#define NP(x) (x ? x : "(null)")

OSyncGroupEnv *genv;
OSyncPluginEnv *penv;

void get_advanced(OSyncPluginConfig *config, const char *name)
{
	cout << "Advanced: " << name << ": "
		<< NP(osync_plugin_config_get_advancedoption_value_by_name(config, name))
		<< endl;
}

void add_advanced(OSyncPluginConfig *config,
			const char *name,
			const char *display,
			const char *value)
{
	OSyncError *ose = NULL;
	OSyncPluginAdvancedOption *option = osync_plugin_advancedoption_new(&ose);
	if( !option ) throw "bad option";
	osync_plugin_advancedoption_set_displayname(option, "Test Display Name");
	osync_plugin_advancedoption_set_name(option, "TestName");
	osync_plugin_advancedoption_set_type(option, OSYNC_PLUGIN_ADVANCEDOPTION_TYPE_STRING);
	osync_plugin_advancedoption_set_value(option, "Whippoorwill");
	osync_plugin_config_add_advancedoption(config, option);
	osync_plugin_advancedoption_unref(option);
}

void dump_resource(OSyncPluginResource *res)
{
	cout << "Resource: "
	     << NP(osync_plugin_resource_get_name(res))
	     << ": "
	     << (osync_plugin_resource_is_enabled(res) ? "enabled" : "disabled");
	if( osync_plugin_resource_get_preferred_format(res) )
		cout << "\n   pref format: "
		     << osync_plugin_resource_get_preferred_format(res);

	if( osync_plugin_resource_get_mime(res) )
		cout << "\n   mime: "
		     << NP(osync_plugin_resource_get_mime(res));

	if( osync_plugin_resource_get_objtype(res) )
		cout << "\n   objtype: "
		     << NP(osync_plugin_resource_get_objtype(res));

	if( osync_plugin_resource_get_path(res) )
		cout << "\n   path: "
		     << NP(osync_plugin_resource_get_path(res));

	if( osync_plugin_resource_get_url(res) )
		cout << "\n   url: "
		     << NP(osync_plugin_resource_get_url(res));
	osync_plugin_resource_set_url(res, "http://whoopiecushion/");

	cout << endl;
}

void dump_resources(OSyncPluginConfig *config)
{
	OSyncList *resources = osync_plugin_config_get_resources(config);
	for( OSyncList *o = resources; o; o = o->next ) {
		OSyncPluginResource *res = (OSyncPluginResource*) o->data;
		dump_resource(res);
	}
}

void test_member(OSyncGroup *group, int member_id)
{
	OSyncError *ose = NULL;

	cout << "Testing member: " << member_id << endl;

	OSyncMember *member = osync_group_find_member(group, member_id);
	if( !member ) throw "bad member id";
	OSyncPlugin *plugin = osync_plugin_env_find_plugin(penv, osync_member_get_pluginname(member));
	if( !plugin ) throw "bad plugin";
	OSyncPluginConfig *config = osync_member_get_config_or_default(member, &ose);
	if( !config ) throw "bad config";

	// extract advanced fields
	get_advanced(config, "PinCode");
	get_advanced(config, "Debug");

	// test add
	add_advanced(config, "TestName", "Test Display Name", "Whippoorwill");
	get_advanced(config, "TestName");

	// extract resources
	dump_resources(config);

	// save new config
	if( !osync_member_save(member, &ose) )
		throw "bad member save";

	// due to leak in library, I don't think this is safe to call?
	//osync_plugin_config_unref(config);
}

void test()
{
	OSyncError *ose = NULL;

	// create
	genv = osync_group_env_new(&ose);
	if( !genv ) throw "bad genv";
	penv = osync_plugin_env_new(&ose);
	if( !penv ) throw "bad penv";

	// load
	if( !osync_group_env_load_groups(genv, NULL, &ose) )
		throw "bad group load";
	if( !osync_plugin_env_load(penv, NULL, &ose) )
		throw "bad plugin load";

	// find config
	OSyncGroup *group = osync_group_env_find_group(genv, "test");
	if( !group ) throw "bad group";
	test_member(group, 1);
	test_member(group, 2);

	// clean up
	osync_plugin_env_unref(penv);
	osync_group_env_unref(genv);
}

int main()
{
	try { test(); }
	catch( const char *msg ) {
		cout << "Exception: " << msg << endl;
		return 1;
	}
}

