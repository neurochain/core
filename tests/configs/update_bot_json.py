#!/usr/bin/env python3
import json

import fire


def update_config(config):
    networking = config.get("networking")
    if networking:
        if "keyPubPath" in networking and "keyPrivPath" in networking:
            networking["keysPaths"] = [
                {
                    "keyPubPath": networking.pop("keyPubPath"),
                    "keyPrivPath": networking.pop("keyPrivPath"),
                }
            ]
        elif "key_pub_path" in networking and "key_priv_path" in networking:
            networking["keys_paths"] = [
                {
                    "key_pub_path": networking.pop("key_pub_path"),
                    "key_priv_path": networking.pop("key_priv_path"),
                }
            ]

    database = config.get("database")
    if database:
        if "block0Format" in database and "block0Path" in database:
            database["block0File"] = {
                "blockFormat": database.pop("block0Format"),
                "blockPath": database.pop("block0Path"),
            }

        elif "block0_format" in database and "block0_path" in database:
            database["block0_file"] = {
                "block_format": database.pop("block0_format"),
                "block_path": database.pop("block0_path"),
            }

    return config


def update_config_files(*filenames):
    for name in filenames:
        print("processing", name)
        with open(name, "r") as config_file:
            config = json.load(config_file)
        with open(name, "w") as config_file:
            json.dump(update_config(config), config_file, indent=2)


if __name__ == "__main__":
    fire.Fire(update_config_files)
