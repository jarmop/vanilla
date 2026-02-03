const std = @import("std");

// const options = std.debug.StackUnwindOptions{
//     // Ignore frames up to a specific address
//     // .ignore_until = some_return_address,
// };

export fn dump_stack_trace() void {
    std.debug.dumpCurrentStackTrace(null);
}

pub const _start = void;
