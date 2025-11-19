# unicorn_syscall_scaffold.py
from unicorn import Uc, UC_ARCH_X86, UC_MODE_64, UcError, UC_HOOK_INSN
from unicorn.x86_const import *
import sys

# 模拟内存布局
ADDRESS = 0x1000000
STACK_ADDR = 0x0ff0000
STACK_SIZE = 0x2000

# 下面这段机器码会调用 syscall（假设缓冲区地址和长度已准备好）
# 为练习目的，我们直接用 bytes 形式的机器码（不需要 keystone）
# 机器码做两次 syscall：一次是 write, 一次是 exit
X86_64_CODE = (
    b"\x48\xC7\xC7\x01\x00\x00\x00"      # mov rdi, 1         ; fd = 1
    b"\x48\xC7\xC6\x00\x10\x00\x00"      # mov rsi, 0x1000000+0x100 (buf addr)
    b"\x48\xC7\xC2\x0d\x00\x00\x00"      # mov rdx, 13        ; len = 13 (TODO: 可改)
    b"\x48\xC7\xC0\x00\x00\x00\x00"      # mov rax, <TODO_WRITE_SYSCALL> ; syscall number (TODO)
    b"\x0F\x05"                          # syscall
    b"\x48\xC7\xC0\x3C\x00\x00\x00"      # mov rax, 60       ; exit syscall
    b"\x48\x31\xFF"                      # xor rdi, rdi      ; exit(0)
    b"\x0F\x05"                          # syscall
)

# buf（要被 write 的字符串）放在模拟内存的偏移处
BUF = b"Hello, unicorn\n"  # 长度 14（示例） — 你可以适配上面的 len

def hook_syscall(mu, user_data):
    """
    这个 hook 会在我们检测到 syscall 指令时被调用（示例中用 UC_HOOK_INTR/UC_HOOK_INSN）。
    你的工作是在这里：
      1) 读取 RAX（系统调用号）
      2) 如果是 write（syscall号 TODO），读取 RDI/RSI/RDX（fd, buf, len）
      3) 从模拟内存读取 buf 并将其打印到宿主 stdout
      4) 将写入字节数写回 RAX（作为返回值）
      5) 将 RIP 前进到 syscall 指令之后（避免再次重复执行）
    注意：unicorn 的寄存器常量在 unicorn.x86_const 中可查
    """
    # TODO: 实现 syscall 处理逻辑
    raise NotImplementedError("请实现 hook_syscall 中的 syscall 处理（只回答第一个问题后我再引导下一步）")

def main():
    mu = Uc(UC_ARCH_X86, UC_MODE_64)
    # 映射内存
    mu.mem_map(ADDRESS, 2 * 1024 * 1024)  # code + data
    mu.mem_map(STACK_ADDR - STACK_SIZE, STACK_SIZE)
    mu.mem_write(ADDRESS, X86_64_CODE)

    # 将 buf 写入内存（buf 在机器码中使用的地址）
    buf_addr = ADDRESS + 0x100
    mu.mem_write(buf_addr, BUF)

    # 设置栈指针
    mu.reg_write(UC_X86_REG_RSP, STACK_ADDR)

    # 添加一个针对 syscall 指令（0x0f05）的钩子（这里用 UC_HOOK_INSN）
    # 注意：部分 unicorn 绑定版本支持按 opcode hook；如果不支持，可改为 UC_HOOK_CODE 并判断指令 bytes
    mu.hook_add(UC_HOOK_INSN, hook_syscall, None, 1, 0, UC_X86_INS_SYSCALL)

    try:
        mu.emu_start(ADDRESS, ADDRESS + len(X86_64_CODE))
    except UcError as e:
        print("Unicorn emulation error:", e)

if __name__ == "__main__":
    main()
