// createApp.ts - "New App" wizard (giong esp-idf "New Project"): hoi bare-metal
// hay FreeRTOS truoc, roi copy <SDK>/tools/templates/ HOAC
// <SDK>/tools/templates_freertos/ (2 nguon THAT nam trong repo, KHONG bundle
// vao extension - sua template khong can rebuild/repackage extension) sang 1
// thu muc moi, thay 2 token trong NOI DUNG file:
//   __APP_NAME__               -> ten app nguoi dung chon
//   __NEON_MDF_RELATIVE_PATH__ -> duong dan tuong doi TU DEST DIR toi SDK
//                                  dang duoc chon (xem ../sdkPath.ts) - TINH
//                                  THAT dua tren vi tri app + SDK, KHONG con
//                                  gia dinh co dinh "../../neon-mdf" nua
//                                  (gia dinh do sai neu nguoi dung bam "Chon
//                                  thu muc khac..." ra ngoai examples/, hoac
//                                  da chon SDK path rieng qua neonMdf.sdkPath).
// roi mo trong cua so moi. Ca 2 template da duoc build-test THAT (ca 3 preset
// FU2/WEACT_F412/CH32V003F4P6, flash+chay that tren WEACT_F412) truoc khi
// tinh nang nay duoc viet - xem CMakeLists.txt/Src/main.c trong
// tools/templates/ va tools/templates_freertos/.
//
// templatesDir TINH TU sdkDir (khong phai tu workspaceFolder dang mo) -
// chuyen tu Projects/Templates/ vao trong SDK 2026-08-10 (day la tai san
// CUA SDK, giong openocd cfg tap trung - xem cmake/openocd/), nen phai doi
// resolveSdkPathUri() cua CHINH destDir de tim dung, cung 1 tinh than voi
// cach sdkDir/relPath duoi day da duoc tinh dung tu truoc.
import * as vscode from "vscode";
import { resolveSdkPathUri, relativePathToSdk, looksLikeSdkRoot } from "../sdkPath";

const APP_NAME_PATTERN = /^[A-Za-z_][A-Za-z0-9_]*$/;

async function pathExists(uri: vscode.Uri): Promise<boolean> {
  try {
    await vscode.workspace.fs.stat(uri);
    return true;
  } catch {
    return false;
  }
}

/**
 * examples/ THAT de tao app moi trong do - tu nhan dien 2 truong hop
 * (2026-08-10, fix bug that: truoc day LUON gia dinh workspace dang mo la 1
 * thu muc app cu the vd examples/Blinky roi lay `..` lam examples/ - neu
 * nguoi dung mo NGUYEN REPO ROOT (neon_mdf/, chua ca neon-mdf/ SDK lan
 * examples/) thay vi 1 app, phep tinh do LECH 1 CAP, tao app o vi tri sai
 * hoan toan - da xac nhan that qua bug report cua nguoi dung, khong phai
 * doan):
 *   1. Workspace dang mo LA 1 thu muc app (vd examples/Blinky) -
 *      examples/ la thu muc CHA cua no (hanh vi cu, GIU NGUYEN).
 *   2. Workspace dang mo LA repo root (co ca neon-mdf/ SDK lan examples/
 *      lam thu muc con) - examples/ la thu muc CON cua no.
 * Uu tien kiem tra (2) truoc vi de xac dinh CHAC CHAN hon (co marker That
 * qua looksLikeSdkRoot() tren `<folder>/neon-mdf`, khong chi dua vao
 * `examples/` co ton tai hay khong - 1 thu muc app cu bat ky cung co the
 * tinh co co thu muc con ten "examples/" khong lien quan). Neu ca 2 khong
 * khop (vd nguoi dung mo 1 thu muc hoan toan khac), fallback ve hanh vi cu
 * (thu muc cha) - khong te hon truoc day, van co canh bao rieng o buoc
 * kiem tra sdkDir ben duoi neu sai that.
 */
async function resolveExamplesDir(openedFolder: vscode.Uri): Promise<vscode.Uri> {
  const childExamples = vscode.Uri.joinPath(openedFolder, "examples");
  const childSdk = vscode.Uri.joinPath(openedFolder, "neon-mdf");
  if ((await pathExists(childExamples)) && (await looksLikeSdkRoot(childSdk))) {
    return childExamples;
  }
  return vscode.Uri.joinPath(openedFolder, "..");
}

/**
 * Copy de quy tu templateDir sang destDir, thay tung token trong `tokens`
 * trong NOI DUNG moi file (khong phai ten file - khong file template nao
 * dung token trong ten). Gia dinh moi file template la text (dung voi toan
 * bo tools/templates/ hien tai: .c/.h/.json/.cfg/txt) - neu sau nay them
 * file nhi phan vao template, ham nay se lam hong no, can sua lai.
 */
async function copyTemplateDir(src: vscode.Uri, dest: vscode.Uri, tokens: Record<string, string>): Promise<void> {
  await vscode.workspace.fs.createDirectory(dest);
  const entries = await vscode.workspace.fs.readDirectory(src);
  for (const [name, type] of entries) {
    const srcChild = vscode.Uri.joinPath(src, name);
    const destChild = vscode.Uri.joinPath(dest, name);
    if (type === vscode.FileType.Directory) {
      await copyTemplateDir(srcChild, destChild, tokens);
    } else {
      const bytes = await vscode.workspace.fs.readFile(srcChild);
      let text = Buffer.from(bytes).toString("utf8");
      for (const [token, value] of Object.entries(tokens)) {
        text = text.split(token).join(value);
      }
      await vscode.workspace.fs.writeFile(destChild, Buffer.from(text, "utf8"));
    }
  }
}

export async function createAppCommand(): Promise<void> {
  const folder = vscode.workspace.workspaceFolders?.[0];
  if (!folder) {
    vscode.window.showWarningMessage(
      "NEON-MDF: chua mo workspace folder nao - mo thu muc goc repo (neon_mdf/) hoac 1 app vi du (vd examples/Blinky/) truoc."
    );
    return;
  }

  const appName = await vscode.window.showInputBox({
    prompt: "Ten app moi (dung lam ten thu muc + ten CMake target - chi chu/so/underscore, khong bat dau bang so)",
    placeHolder: "vd my_app",
    validateInput: (value) => {
      if (!value) {
        return "Ten app khong duoc de trong";
      }
      if (!APP_NAME_PATTERN.test(value)) {
        return "Chi cho phep chu/so/underscore, khong bat dau bang so (vd my_app)";
      }
      return undefined;
    },
  });
  if (!appName) {
    return;
  }

  // App thuong (bare-metal, superloop) vs app FreeRTOS (scheduler that,
  // xTaskCreate/vTaskStartScheduler) - 2 bo file NGUON KHAC NHAU (CMakeLists.txt/
  // Src/main.c/Inc/FreeRTOSConfig.h) chu khong phai 1 flag runtime, vi
  // FreeRTOS can them include(cmake/freertos.cmake)/neon_mdf_add_freertos()
  // + glue arch-specific (SVC/PendSV/SysTick cho WEACT_F412, STK/mtvec cho
  // CH32V003F4P6 - xem tools/templates_freertos/Src/main.c) khong the "tat"
  // don gian bang #ifdef trong 1 template chung. Xem tools/templates/ (bare-
  // metal) vs tools/templates_freertos/ (FreeRTOS) - ca 2 deu la template
  // TOI GIAN nhu nhau (khong GPIO/LED, chi proof-of-life), khac nhau THUAN
  // TUY o co scheduler hay khong.
  const templateChoice = await vscode.window.showQuickPick(
    [
      {
        label: "$(circuit-board) App thuong (bare-metal)",
        description: "superloop don gian, khong FreeRTOS",
        templateFolder: "templates",
      },
      {
        label: "$(list-tree) App dung FreeRTOS",
        description: "1 task + scheduler, xem examples/FreeRTOS_Blinky/ cho vi du day du hon",
        templateFolder: "templates_freertos",
      },
    ],
    { placeHolder: "Tao app kieu gi?" }
  );
  if (!templateChoice) {
    return;
  }

  const examplesDir = await resolveExamplesDir(folder.uri);
  const defaultDest = vscode.Uri.joinPath(examplesDir, appName);

  const picked = await vscode.window.showQuickPick(
    [
      { label: "$(folder) Tao trong examples/ (khuyen dung)", description: defaultDest.fsPath, dest: defaultDest },
      { label: "$(folder-opened) Chon thu muc khac...", description: "duyet thu muc", dest: undefined },
    ],
    { placeHolder: "Tao app o dau?" }
  );
  if (!picked) {
    return;
  }

  let destDir = picked.dest;
  if (!destDir) {
    const chosen = await vscode.window.showOpenDialog({
      canSelectFolders: true,
      canSelectFiles: false,
      canSelectMany: false,
      openLabel: "Tao app o day",
    });
    if (!chosen || chosen.length === 0) {
      return;
    }
    destDir = vscode.Uri.joinPath(chosen[0], appName);
  }

  if (await pathExists(destDir)) {
    vscode.window.showErrorMessage(`NEON-MDF: thu muc da ton tai: ${destDir.fsPath}`);
    return;
  }

  // SDK path TINH THAT dua tren vi tri destDir (khong phai workspaceFolder
  // dang mo) - khop voi cach CMakeLists.txt cua app moi se tu tinh
  // (${CMAKE_CURRENT_SOURCE_DIR} = destDir luc build that). Dung setting
  // neonMdf.sdkPath neu nguoi dung da chon, khong thi quy uoc cu (2 cap len
  // neon-mdf/) - xem sdkPath.ts.
  const sdkDir = resolveSdkPathUri(destDir);
  if (!(await looksLikeSdkRoot(sdkDir))) {
    const proceed = await vscode.window.showWarningMessage(
      `NEON-MDF: khong tim thay neon-mdf SDK tai ${sdkDir.fsPath} (thu muc nay khong giong 1 checkout SDK that). Neu app dang tao o vi tri khong chuan (khong nam duoi examples/), hay chon SDK path rieng qua lenh "NEON-MDF: Chon Duong Dan SDK..." truoc. Van tao app (co the build se loi)?`,
      "Van tao",
      "Huy"
    );
    if (proceed !== "Van tao") {
      return;
    }
  }

  const templatesDir = vscode.Uri.joinPath(sdkDir, "tools", templateChoice.templateFolder);
  if (!(await pathExists(templatesDir))) {
    vscode.window.showErrorMessage(`NEON-MDF: khong tim thay thu muc template: ${templatesDir.fsPath}`);
    return;
  }

  const relPath = relativePathToSdk(destDir, sdkDir);

  await copyTemplateDir(templatesDir, destDir, {
    __APP_NAME__: appName,
    __NEON_MDF_RELATIVE_PATH__: relPath,
  });

  const openNow = await vscode.window.showInformationMessage(
    `NEON-MDF: da tao app "${appName}" tai ${destDir.fsPath}`,
    "Mo trong cua so moi"
  );
  if (openNow) {
    await vscode.commands.executeCommand("vscode.openFolder", destDir, { forceNewWindow: true });
  }
}
