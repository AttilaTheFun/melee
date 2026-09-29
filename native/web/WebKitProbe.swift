import UIKit
import WebKit
@main final class App: UIResponder, UIApplicationDelegate {
 func application(_ application:UIApplication,configurationForConnecting session:UISceneSession,options:UIScene.ConnectionOptions)->UISceneConfiguration {
  let config=UISceneConfiguration(name:"Probe",sessionRole:session.role);config.delegateClass=Scene.self;return config
 }
}
final class Scene:UIResponder,UIWindowSceneDelegate,WKScriptMessageHandler {
 var window:UIWindow?
 func scene(_ scene:UIScene,willConnectTo session:UISceneSession,options:UIScene.ConnectionOptions) {
  guard let scene=scene as? UIWindowScene else{return}
  let configuration=WKWebViewConfiguration();configuration.userContentController.add(self,name:"report")
  let web=WKWebView(frame:.zero,configuration:configuration)
  let controller=UIViewController();controller.view=web
  window=UIWindow(windowScene:scene);window?.rootViewController=controller;window?.makeKeyAndVisible()
  web.load(URLRequest(url:URL(string:ProcessInfo.processInfo.environment["MELEE_WEB_URL"]!)!))
 }
 func userContentController(_ c:WKUserContentController,didReceive message:WKScriptMessage) {
  let url=FileManager.default.urls(for:.documentDirectory,in:.userDomainMask)[0].appendingPathComponent("result.json")
  if let data=try? JSONSerialization.data(withJSONObject:message.body,options:[.prettyPrinted]){try? data.write(to:url)}
 }
}
