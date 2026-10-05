import time
from datetime import datetime


class Bcolors:
    OKCYAN = "\033[96m"
    OKGREEN = "\033[92m"
    WARNING = "\033[93m"
    FAIL = "\033[91m"
    ENDC = "\033[0m"
    BOLD = "\033[1m"


class Utils:
    @classmethod
    def log(cls, message: str) -> None:
        print(
            f"{Bcolors.OKGREEN}[branch-and-bound@1.0.0 | {cls.get_timestamp()}] - Ok: ",
            end="",
        )
        for char in message:
            print(char, end="", flush=True)
            time.sleep(0.01)
        print("\n", end="")

    @classmethod
    def error(cls, message: str) -> None:
        print(
            f"{Bcolors.FAIL}[branch-and-bound@1.0.0 | {cls.get_timestamp()}] - Error: ",
            end="",
        )
        for char in message:
            print(char, end="", flush=True)
            time.sleep(0.01)
        print("\n", end="")

    @classmethod
    def warning(cls, message: str) -> None:
        print(
            f"{Bcolors.WARNING}[branch-and-bound@1.0.0 | {cls.get_timestamp()}] - Warning: ",
            end="",
        )
        for char in message:
            print(char, end="", flush=True)
            time.sleep(0.01)
        print("\n", end="")

    @classmethod
    def get_timestamp(cls) -> str:
        return datetime.now().strftime("%Y-%m-%d %H:%M:%S")
