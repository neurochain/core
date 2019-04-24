#!/usr/bin/python3

import pprint
import boto3
import fabric
import fire
from pprint import pprint


class Ec2:
    def __init__(self):
        self.ec2 = boto3.client("ec2")

    def describe_instances_by_role(self, role):
        return self.ec2.describe_instances(
            Filters=[{"Name": "tag:role", "Values": [role]}]
        )

    def describe_instance(self, instance_id):
        print(self.ec2.describe_instances())

    def filter_tags(self, instances, tags):
        instances = []
        for reservation in instances["Reservations"]:
            for instance in reservation["Instances"]:
                element = {tag: instance[tag] for tag in tags}

                instances.append(element)

        return instances

    def instance_tags(self):
        return ["InstanceId", "PublicIpAddress"]

    def filter_instance_tags(self, instances):
        return self.filter_tags(instances, self.instance_tags())

    def boots(self):
        instances = self.describe_instances_by_role("boot")
        return self.filter_tags(instances, self.instance_tags())

    def bots(self):
        instances = self.describe_instances_by_role("bot")
        return self.filter_tags(instances, self.instance_tags())


class RemoteExec:
    def __init__(self, ips, user="ubuntu", key_path="testnet.pem"):
        self.user = user
        self.key_path = key_path
        self.ips = ips

    def execute(self, commands):
        result = fabric.ThreadingGroup(
            *self.ips,
            user=self.user,
            connect_kwargs={"key_filename": self.key_path},
        ).run(command, hide="both", warn=True)
        for connection, result in result.items():
            print(connection.host.strip(), result.stdout.strip(), result.stderr)


class Cli:
    def __init__(self, key_path="key.pem"):
        self.ec2 = Ec2()
        self.key_path = key_path

    def boots(self):
        print(self.ec2.boots())

    def bots(self):
        print(self.ec2.bots())

    def execute(self, role, *commands):
        instances = self.ec2.filter_instance_tags(
            self.ec2.describe_instances_by_role(role)
        )
        pprint(instances)
        remote = RemoteExec(
            [instance["PublicIpAddress"] for instance in instances],
            key_path=self.key_path,
        )
        remote.execute(command)

    def docker_install(self, role):
        self.execute(role=role, command='sudo apt-get remove docker docker-engine docker.io containerd runc')
        self.execute(role=role, command='sudo apt-get update')
        self.execute(role=role, command='sudo apt-get install apt-transport-https ca-certificates curl gnupg-agent software-properties-common')
        self.execute(role=role, command='curl -fsSL https://download.docker.com/linux/ubuntu/gpg | sudo apt-key add -')
        self.execute(role=role, command='sudo apt-key fingerprint 0EBFCD88')
        self.execute(role=role, command='sudo add-apt-repository "deb [arch=amd64] https://download.docker.com/linux/ubuntu $(lsb_release -cs) stable"')
        self.execute(role=role, command='sudo apt-get update')
        self.execute(role=role, command='sudo apt-get install docker-ce docker-ce-cli containerd.io')
                     
        
    def docker_init(self, role):
        self.execute(role=role, command='docker network create core')
        self.execute(role=role, command='docker network create core')

    def describe_instance(self, instance_id):
        self.ec2.describe_instance(instance_id)

    def core_update(self, role):
        self.execute(role=role, command="docker container pull ")

if __name__ == "__main__":
    fire.Fire(Cli)
