/*
 * Copyright 2022 Matt Jones
 * Date: 2023.11.20
 * Desc: Allow access to the API from other domains (also useful for local dev).
 */

package zone.mattjones.trainsignal;

        import jakarta.servlet.FilterChain;
        import jakarta.servlet.ServletException;
        import jakarta.servlet.annotation.WebFilter;
        import jakarta.servlet.http.HttpFilter;
        import jakarta.servlet.http.HttpServletRequest;
        import jakarta.servlet.http.HttpServletResponse;

        import java.io.IOException;

@WebFilter(urlPatterns = "/*", filterName = "CorsFilter")
public class CorsFilter extends HttpFilter {
    @Override
    public void doFilter(HttpServletRequest req, HttpServletResponse res,
            FilterChain chain) throws IOException, ServletException {
        res.setHeader("Access-Control-Allow-Origin", "*");
        res.setHeader("Access-Control-Allow-Methods", "GET");

        super.doFilter(req, res, chain);
    }
}
