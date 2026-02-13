# foo_gsp
[![GithubActions_badge]][GithubActions_link]

foobar2000 のアクティブプレイリストを JSON にして、Google Apps Script (GAS) の Web アプリへ `HTTP POST` するコンポーネントです。

[GithubActions_badge]: https://github.com/sanadamushitaro-gif/foo_gsp/actions/workflows/ci.yaml/badge.svg
[GithubActions_link]: https://github.com/sanadamushitaro-gif/foo_gsp/actions

## 現在の仕様
- foobar2000 メニュー `File > Upload playlist` から送信
- 送信データ: `title` / `artist` / `album` / `play_count`
- GAS の Web アプリ URL は固定ではなく、`Preferences > Tools > foo_gsp` の `GSP deployment token` から組み立て
- GAS 側のサンプルスクリプト（`gsp_script.txt`）は毎回シートを上書き

## Build
1. [foobar2000 SDK](https://www.foobar2000.org/SDK) を取得し、`lib\foobar2000_sdk` に展開
2. `lib\foobar2000_sdk\foobar2000\helpers\foobar2000_sdk_helpers.vcxproj` を開き、`wtl.targets` の相対パスをこのリポジトリ構成に合わせて修正
3. `foo_gsp.sln` を Visual Studio で開く
4. `foo_gsp` をビルド
5. 生成された `foo_gsp.dll` を `foobar2000\components` に配置

## GAS 側セットアップ
1. `gsp_script.txt` の内容を Apps Script に貼り付け
2. スプレッドシートを紐づけた状態で Web アプリとしてデプロイ
3. デプロイ後に表示されるデプロイID（`AKfy...`）を控える
4. スクリプト更新時は再デプロイし、新しいデプロイIDを使う

## foobar2000 側セットアップ
1. `File > Preferences > Tools > foo_gsp` を開く
2. `GSP deployment token` に `AKfy...` のみを入力して Apply
3. `File > Upload playlist` を実行

## 送信 JSON 例
```json
{
  "tracks": [
    {
      "title": "Song A",
      "artist": "Artist A",
      "album": "Album A",
      "play_count": "12"
    }
  ]
}
```

## トラブルシュート
- `POST ok` だが HTML エラーページが返る:
  - トークンではなく URL 全体を入力していないか確認
  - Web アプリのデプロイIDが最新か確認
  - Web アプリの公開設定（アクセス権）を確認
- `GSP token is not configured`:
  - `Preferences > Tools > foo_gsp` でトークンを設定して Apply

## Useful references
- [Visual Studio Compatibility](https://wiki.hydrogenaud.io/index.php?title=Foobar2000:Development:Visual_Studio_Compatibility)
- [foobar2000 development tutorial](https://yirkha.fud.cz/tmp/496351ef.tutorial-draft.html)
- [Resources for foobar2000 component developers](https://foosion.foobar2000.org/developers/)
