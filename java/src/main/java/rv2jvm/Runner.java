package rv2jvm;

import java.util.Arrays;

public class Runner {
    public static void main(String[] args) {
        System.out.println("Running RvRuntime\n- - -");
        RvRuntime.main(args);
        System.out.println("RvRuntime finished\n- - -");
        System.out.println("Registers: " + Arrays.toString(RvRuntime.registers));
    }
}
