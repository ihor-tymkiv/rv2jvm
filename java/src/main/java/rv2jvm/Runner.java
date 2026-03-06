package rv2jvm;

import java.util.Arrays;

public class Runner {
    public static void main(String[] args) {
        System.out.println("= = =");
        System.out.println("- - - | RvRuntime started");
        RvRuntime.main(args);
        System.out.println("- - - | RvRuntime finished");
        System.out.println("Registers: " + Arrays.toString(RvRuntime.registers));
        System.out.println("Memory: " + Arrays.toString(RvRuntime.memory));
        System.out.println("= = =");
    }
}
