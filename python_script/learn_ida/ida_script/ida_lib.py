import idapro
# 这些模块是运行时注入的；IDE 报红不影响
import ida_auto, ida_name, ida_bytes, ida_kernwin, idautils, ida_loader, ida_nalt  # noinspection PyUnresolvedReferences
from pathlib import Path
import os

cur_file=__file__

INPUT =os.path.join(os.path.join(os.path.abspath(cur_file),"../..") ,"lib/libollvm_demo.so")    # 也可以直接给 .i64
SAVE_AS = None                      # 想"另存为"就给路径，比如 r"D:\work\libavutil_lab.i64"

def open_and_wait(inp: str):
    rc = idapro.open_database(inp, True)  # 返回码不可靠，别用它判断成功与否
    ida_auto.auto_wait()
    # 实证校验
    # 用 ida_nalt 获取当前数据库对应的输入文件名，作为"成功打开"的实证
    root = ida_nalt.get_root_filename()
    print("root filename:", root)
    return bool(root)

def rename_some():
    # 例1：把第一个函数改名
    first = next(iter(idautils.Functions()))
    ida_name.set_name(first, "my_first_func", ida_name.SN_CHECK)

    # 例2：给所有以 "sub_" 开头的函数加前缀（示例：sub_* -> lab_sub_*)
    for ea in idautils.Functions():
        old = ida_name.get_name(ea)
        if old and old.startswith("sub_"):
            ida_name.set_name(ea, "lab_" + old, ida_name.SN_CHECK)

def add_comments():
    any_fn = next(iter(idautils.Functions()))
    ida_bytes.set_cmt(any_fn, "renamed by script", 0)  # 行尾注释
    ida_bytes.set_cmt(any_fn, "top comment", 1)        # 前置注释

def patch_demo():
    # ⚠️ 示例：把第一个函数的前4字节打成 AArch64 NOP（\x1F\x20\x03\xD5）
    ea = next(iter(idautils.Functions()))
    ida_bytes.patch_bytes(ea, b"\x1f\x20\x03\xd5")

def save_idb(save_as: str | None):
    if save_as:
        Path(save_as).parent.mkdir(parents=True, exist_ok=True)
        ida_loader.save_database(save_as, 0)  # 另存为新 .i64
        print("saved:", save_as)
    else:
        ida_loader.save_database("", 0)       # 保存当前数据库
        print("saved current IDB")

def main():
    open_and_wait(INPUT)
    rename_some()
    add_comments()
    patch_demo()
    save_idb(SAVE_AS)
    idapro.close_database()

if __name__ == "__main__":
    main()