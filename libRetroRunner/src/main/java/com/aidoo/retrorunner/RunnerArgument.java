package com.aidoo.retrorunner;

import java.util.HashMap;

public class RunnerArgument {
    private String romPath;
    private String corePath;

    private String systemPath;
    private String savePath;

    private final HashMap<String, String> variables = new HashMap<>();

    public String getRomPath() {
        return romPath;
    }

    public void setRomPath(String romPath) {
        this.romPath = romPath;
    }

    public String getCorePath() {
        return corePath;
    }

    public void setCorePath(String corePath) {
        this.corePath = corePath;
    }

    public String getSystemPath() {
        return systemPath;
    }

    public void setSystemPath(String systemPath) {
        this.systemPath = systemPath;
    }

    public String getSavePath() {
        return savePath;
    }

    public void setSavePath(String savePath) {
        this.savePath = savePath;
    }

    public boolean haveVariables(){
        return !variables.isEmpty();
    }

    public HashMap<String, String> getVariables() {
        return variables;
    }

    public void clearVariables() {
        this.variables.clear();
    }

    public void setVariables(HashMap<String, String> variables) {
        this.variables.putAll(variables);
    }
}
