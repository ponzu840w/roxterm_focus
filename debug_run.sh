#!/bin/bash

# ログファイルのパス
LOG_FILE="$HOME/.local/var/log/roxterm-forcus.log"

# 起動時刻をログに出力
echo "---" >> "$LOG_FILE"
echo "$(date): Starting roxterm-focus daemon." >> "$LOG_FILE"

# デーモンをバックグラウンドで起動し、標準出力と標準エラー出力をログファイルに追記
# exec を使うことで、このスクリプトのプロセスIDがデーモンのプロセスIDに置き換わります
/usr/local/bin/roxterm_focus >> "$LOG_FILE" 2>&1 &

# バックグラウンドで起動したデーモンのプロセスIDを取得
PID=$!

# デーモンの終了を待つ
wait $PID

# 終了時刻と終了コードをログに出力
echo "$(date): roxterm-focus daemon finished with exit code $?." >> "$LOG_FILE"
