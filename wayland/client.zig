const std = @import("std");
const linux = std.os.linux;
const allocator = std.heap.c_allocator;

fn connect_server() !i32 {
    // zig implementation of linux.socket return usize which matches the address space of the system,
    // so 64-bit in my case, so need to cast to 32-bit int which is expected by the connect syscall.
    const fd: i32 = @intCast(linux.socket(linux.AF.UNIX, linux.SOCK.STREAM | linux.SOCK.CLOEXEC, 0));

    var path: [108]u8 = @splat(0);
    var env_map = try std.process.getEnvMap(allocator);
    defer env_map.deinit();
    const runtime = env_map.get("XDG_RUNTIME_DIR").?;
    const display = env_map.get("WAYLAND_DISPLAY").?;
    const path_len = runtime.len + 1 + display.len;
    @memcpy(path[0..runtime.len], runtime);
    @memcpy(path[runtime.len..(runtime.len + 1)], "/");
    @memcpy(path[(runtime.len + 1)..(path_len)], display);
    const addr: linux.sockaddr.un = .{ .family = linux.AF.UNIX, .path = path };
    // std.debug.print("{s}\n", .{path});

    // For some reason, can't cast directly to i32, but has to cast first to integer of usize
    // and then cast that to the integer size required by the connect function
    const connectError64: i64 = @bitCast(linux.connect(fd, &addr, @sizeOf(linux.sockaddr.un)));
    const connectError32: i32 = @intCast(connectError64);
    if (connectError32 < 0) {
        std.debug.print("Connect error: {}\n", .{connectError32});
        std.os.linux.exit(1);
    }

    return fd;
}

fn get_registry(fd: i32) void {
    const msg = [_]u8{ 1, 0, 0, 0, 1, 0, 12, 0, 2, 0, 0, 0 };

    const n = linux.write(fd, &msg, msg.len);

    std.debug.print("Wrote {} bytes\n", .{n});
}

// const Header = extern struct { object_id: u32, opcode: u16, size: u16 };
const Header = packed struct { object_id: u32, opcode: u16, size: u16 };

pub fn main() !void {
    const fd = try connect_server();
    std.debug.print("fd: {}\n", .{fd});

    get_registry(fd);

    // var header: [8]u8 = @splat(0);
    var header: Header = undefined;
    // const n = linux.read(fd, &header, header.len);
    const n = linux.read(fd, @ptrCast(&header), @sizeOf(Header));

    std.debug.print("Read {} bytes\n", .{n});
    std.debug.print("Payload size: {}\n", .{header.size});

    // const payload_len = header
}
