#include <libconfig.h>

#include "config.h"

char *get_config_opt(const char *option) {
	config_t cfg;
  	config_setting_t *setting;
  	const char *str;

  	config_init(&cfg);
	
	if (!config_read_file(&cfg, "example.conf")) {
		// could not read config
	}

	if (config_lookup_string(&cfg, option, &str))
    	return (char *)str;
}