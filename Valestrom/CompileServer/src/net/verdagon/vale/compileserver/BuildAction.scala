package net.verdagon.vale.compileserver

import com.google.cloud.functions.{HttpFunction, HttpRequest, HttpResponse}
import net.verdagon.vale.{Err, Ok}
import net.verdagon.vale.driver.Driver
import net.verdagon.vale.driver.Driver.{Options, build, jsonifyProgram}

class BuildAction extends HttpFunction {
  override def service(request: HttpRequest, response: HttpResponse): Unit = {
    val code = scala.io.Source.fromInputStream(request.getInputStream).mkString
    if (code.isEmpty) {
      response.setStatusCode(400)
      response.getWriter.write("To compile a Vale program, specify the Vale code in the request body.\nExample: fn main() int export { 42 }\n")
      return
    }

    val json =
      Driver.build(Options(List("in.vale"), Some(""), None, None, None, false), List(code)) match {
        case Ok(Some(programH)) => jsonifyProgram(programH)
        case Err(error) => {
          response.setStatusCode(400)
          response.getWriter.write(error)
          return
        }
      }

    response.getWriter.write(json + "\n")
  }
}
