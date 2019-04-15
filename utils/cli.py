#!/usr/bin/python3

import pprint
import boto3
import fabric
import fire
from pprint import pprint


class Ec2:
    def __init__(self):
        self.ec2 = boto3.client("ec2")

    def list_vm_by_role(self, role):
        return self.ec2.describe_instances(
            Filters=[{"Name": "tag:role", "Values": [role]}]
        )

    def filter_tags(self, instances, tags):
        list = []
        for reservation in instances["Reservations"]:
            for instance in reservation["Instances"]:
                element = {}
                for tag in tags:
                    element[tag] = instance[tag]

                list.append(element)

        return list

    def instance_tags(self):
        return ["InstanceId", "PublicIpAddress"]

    def filter_instance_tags(self, instances):
        return self.filter_tags(instances, self.instance_tags())

    def boots(self):
        instances = self.list_vm_by_role("boot")
        return self.filter_tags(instances, self.instance_tags())

    def bots(self):
        instances = self.list_vm_by_role("bot")
        return self.filter_tags(instances, self.instance_tags())


class RemoteExec:
    def __init__(self, ips, user="ubuntu", key_path="testnet.pem"):
        self.user = user
        self.key_path = key_path
        self.ips = ips

    def execute(self, command):
        result = fabric.ThreadingGroup(
            *self.ips, user=self.user, connect_kwargs={"key_filename": self.key_path}
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

    def execute(self, role, command):
        instances = self.ec2.filter_instance_tags(self.ec2.list_vm_by_role(role))
        pprint(instances)
        remote = RemoteExec(
            [instance["PublicIpAddress"] for instance in instances],
            key_path=self.key_path,
        )
        remote.execute(command)


if __name__ == "__main__":
    fire.Fire(Cli)
