package su.xash.featureful;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.ComponentName;
import android.content.Intent;
import android.content.SharedPreferences;
import android.net.Uri;
import android.os.Bundle;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputMethodManager;
import android.widget.Button;
import android.widget.EditText;

public class MainActivity extends Activity {
    private SharedPreferences prefs;
    private static final String PREF_NAME = "prefs";
    private static final String KEY_ARGV = "last_argv";

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        View root = findViewById(R.id.layout_launcher);
        if (root != null) {
            root.setFitsSystemWindows(true);
            root.requestFocus();
        }

        prefs = getSharedPreferences(PREF_NAME, MODE_PRIVATE);

        EditText argvInput = findViewById(R.id.cmdArgs);
        Button runButton = findViewById(R.id.button_launch);

        argvInput.setText(prefs.getString(KEY_ARGV, "-console -log"));
        argvInput.setSingleLine(true);
        argvInput.setImeOptions(EditorInfo.IME_ACTION_DONE);
        
        argvInput.setOnEditorActionListener((v, actionId, event) -> {
            if (actionId == EditorInfo.IME_ACTION_DONE) {
                saveArgs(argvInput.getText().toString());
                argvInput.clearFocus();
                InputMethodManager imm = (InputMethodManager) getSystemService(INPUT_METHOD_SERVICE);
                if (imm != null) imm.hideSoftInputFromWindow(v.getWindowToken(), 0);
                return true;
            }
            return false;
        });

        runButton.setOnClickListener(v -> {
            String userArgs = argvInput.getText().toString();
            saveArgs(userArgs);
            startGame(userArgs.trim());
        });
    }

    private void saveArgs(String args) {
        prefs.edit().putString(KEY_ARGV, args).apply();
    }

    private void startGame(String argv) {
        String pkg = "su.xash.engine.test";
        try {
            getPackageManager().getPackageInfo(pkg, 0);
        } catch (Exception e) {
            try {
                pkg = "su.xash.engine";
                getPackageManager().getPackageInfo(pkg, 0);
            } catch (Exception ex) {
                new AlertDialog.Builder(this)
                    .setTitle(R.string.engine_not_found_title)
                    .setMessage(R.string.engine_not_found_msg)
                    .setPositiveButton(R.string.yes, (dialog, which) -> {
                        startActivity(new Intent(Intent.ACTION_VIEW, 
                            Uri.parse("https://github.com/FWGS/xash3d-fwgs/releases/tag/continuous")));
                    })
                    .setNegativeButton(R.string.no, null)
                    .show();
                return;
            }
        }

        Intent intent = new Intent();
        intent.setComponent(new ComponentName(pkg, "su.xash.engine.XashActivity"));
        intent.putExtra("gamedir", "featureful");
        intent.putExtra("gamelibdir", getApplicationInfo().nativeLibraryDir);
        intent.putExtra("argv", argv);
        intent.putExtra("package", getPackageName());
        intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        startActivity(intent);
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        getMenuInflater().inflate(R.menu.main_menu, menu);
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem menuItem) {
        int id = menuItem.getItemId();
        if (id == R.id.action_github) {
            startActivity(new Intent(Intent.ACTION_VIEW, Uri.parse("https://github.com/FreeSlave/halflife-featureful/tree/featureful")));
            return true;
        }
        if (id == R.id.action_discord) {
            startActivity(new Intent(Intent.ACTION_VIEW, Uri.parse("https://discord.gg/A7ZErQAjfa")));
            return true;
        }
        return super.onOptionsItemSelected(menuItem);
    }
}
