#!/usr/bin/env python3

from pathlib import Path

import sh


def start_bots():
    home = str(Path.home())
    bots = []

    for i in range(3):
        getattr(sh.mongo, "neuro133" + str(7 + i))(eval="db.dropDatabase()")

        bots.append(
            sh.Command("~/core/build/src/main")(
                c=home + f"/core/testnet/b{i}.json",
                _bg=True,
                _err_to_out=True,
                _out=home + f"/core/testnet/b{i}.log",
            )
        )

    for bot in bots:
        bot.join()


if __name__ == "__main__":
    start_bots()
