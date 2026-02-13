# foo_gsp
foobar2000 のアクティブプレイリストを JSON 化し、Google Apps Script (GAS) の Web アプリへ HTTP POST するコンポーネントです。

[![GithubActions_badge]][GithubActions_link]

[GithubActions_badge]: https://github.com/sanadamushitaro-gif/foo_gsp/actions/workflows/ci.yaml/badge.svg
[GithubActions_link]: https://github.com/sanadamushitaro-gif/foo_gsp/actions

## できること
- foobar2000 の `File` メニューに `Upload playlist` を追加
- アクティブプレイリストの各曲情報を取得
- `title` / `artist` / `album` / `play_count` を JSON で送信
- 送信先は `src/mainmenu.cpp` の `kUrl`（GAS エンドポイント）

## Usage
1. Get [foobar2000 SDK](https://www.foobar2000.org/SDK) and extract it into `lib\foobar2000_sdk` directory \
   The SDK version used at the time is `2023-04-18`.
2. Use a text editor and open `lib\foobar2000_sdk\foobar2000\helpers\foobar2000_sdk_helpers.vcxproj` \
   To accommodate the directory structure, do the followings:
   * Replace this line from:
     ```xml
     <Import Project="..\packages\wtl.10.0.10320\build\native\wtl.targets" Condition="Exists('..\packages\wtl.10.0.10320\build\native\wtl.targets')" />
     ```
     To:
     ```xml
     <Import Project="..\..\..\..\packages\wtl.10.0.10320\build\native\wtl.targets" Condition="Exists('..\..\..\..\packages\wtl.10.0.10320\build\native\wtl.targets')" />
     ```
   * Replace this line from:
     ```xml
     <Error Condition="!Exists('..\packages\wtl.10.0.10320\build\native\wtl.targets')" Text="$([System.String]::Format('$(ErrorText)', '..\packages\wtl.10.0.10320\build\native\wtl.targets'))" />
     ```
     To:
     ```xml
     <Error Condition="!Exists('..\..\..\..\packages\wtl.10.0.10320\build\native\wtl.targets')" Text="$([System.String]::Format('$(ErrorText)', '..\..\..\..\packages\wtl.10.0.10320\build\native\wtl.targets'))" />
     ```
3. Open `foo_gsp.sln`, select `OK` when asked whether to update compiler/libraries for old projects
4. Build `foo_gsp`!
5. `foo_gsp.dll` will be generated at `Debug`, `Release` or `x64` directory depending on your configuration
6. To load the component in foobar2000, copy the `foo_gsp.dll` into `foobar2000\components` directory

## 実行方法
1. foobar2000 で対象プレイリストをアクティブにする
2. `File > Upload playlist` を実行する
3. foobar2000 の Console で送信 JSON とレスポンスを確認する

## JSON 例
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

## Notes
- GAS の公開 URL は `src/mainmenu.cpp` 内の `kUrl` を書き換えて利用してください。
- `src/upload_http.cpp` は現在のプロジェクト設定ではビルド対象外です。

## Useful references
* [Visual Studio Compatibility](https://wiki.hydrogenaud.io/index.php?title=Foobar2000:Development:Visual_Studio_Compatibility)
* [foobar2000 development tutorial](https://yirkha.fud.cz/tmp/496351ef.tutorial-draft.html)
* [Resources for foobar2000 component developers](https://foosion.foobar2000.org/developers/)
