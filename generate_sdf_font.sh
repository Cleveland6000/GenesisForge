#!/bin/bash
# SDFフォントアトラスを生成するGit Bashスクリプト

# スクリプト開始時の現在のディレクトリを保存
ORIGINAL_DIR="$(pwd)"

# --- 設定 ---
# 元のフォントファイルへのパス (あなたのPC上の実際のパスに設定してください)
FONT_FILE="/c/Users/rkowp/Documents/Fonts/Noto_Sans_JP/NotoSansJP-VariableFont_wght.ttf"

# 文字セットファイルへのパス (プロジェクトルートからの相対パス)
CHARSET_FILE="${ORIGINAL_DIR}/charset.txt" # 絶対パスに変更

# 出力ディレクトリ (プロジェクトルートからの相対パス)
OUTPUT_DIR="${ORIGINAL_DIR}/assets/fonts/" # 絶対パスに変更

# 出力ファイル名 (拡張子なし)
ATLAS_FILENAME="noto_sans_jp_atlas"

# --- MSDF-BMFONT-XML コマンド ---
# 出力ディレクトリが存在しない場合は作成
mkdir -p "${OUTPUT_DIR}"

echo ""
echo "=== SDFフォントアトラスの生成を開始します ==="
echo "  フォントファイル: ${FONT_FILE}"
echo "  文字セットファイル: ${CHARSET_FILE}"
echo "  出力先: ${OUTPUT_DIR}${ATLAS_FILENAME}.png / .json"
echo ""

# デバッグ: msdf-bmfont に渡される最終的な出力ファイルパスを表示
echo "デバッグ: msdf-bmfont に渡される最終的なファイル名パス: ${OUTPUT_DIR}${ATLAS_FILENAME}"
echo ""

# msdf-bmfont コマンドを実行
# --output オプションを削除し、--filename に絶対パスを含める
msdf-bmfont \
    --charset-file "${CHARSET_FILE}" \
    --texture-size 2048,2048 \
    --field-type msdf \
    --distance-range 4 \
    --output-type json \
    --filename "${OUTPUT_DIR}${ATLAS_FILENAME}" \
    --font-size 64 \
    "${FONT_FILE}"

if [ $? -ne 0 ]; then
    echo ""
    echo "エラーが発生しました。上記メッセージを確認してください。"
else
    echo ""
    echo "SDFフォントアトラスの生成が完了しました！"
fi

read -p "続行するにはEnterキーを押してください..."
