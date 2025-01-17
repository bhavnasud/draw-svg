#include "software_renderer.h"

#include <cmath>
#include <vector>
#include <iostream>
#include <algorithm>

#include "triangulation.h"

using namespace std;

namespace CS248 {


// Implements SoftwareRenderer //

// fill a sample location with color
void SoftwareRendererImp::fill_sample(int sx, int sy, const Color &color) {
  // Task 2: implement this function
  // check bounds
	if (sx < 0 || sx >= width * sample_rate) return;
	if (sy < 0 || sy >= height * sample_rate) return;

	Color sample_color;
	float inv255 = 1.0 / 255.0;
	sample_color.r = sample_buffer->at(4 * (sx + sy * width * sample_rate)) * inv255;
	sample_color.g = sample_buffer->at(4 * (sx + sy * width * sample_rate) + 1) * inv255;
	sample_color.b = sample_buffer->at(4 * (sx + sy * width * sample_rate) + 2) * inv255;
	sample_color.a = sample_buffer->at(4 * (sx + sy * width * sample_rate) + 3) * inv255;

	sample_color = ref->alpha_blending_helper(sample_color, color);

	sample_buffer->at(4 * (sx + sy * width * sample_rate)) = (uint8_t)(sample_color.r * 255);
	sample_buffer->at(4 * (sx + sy * width * sample_rate) + 1) = (uint8_t)(sample_color.g * 255);
	sample_buffer->at(4 * (sx + sy * width * sample_rate) + 2) = (uint8_t)(sample_color.b * 255);
	sample_buffer->at(4 * (sx + sy * width * sample_rate) + 3) = (uint8_t)(sample_color.a * 255);

}

// fill samples in the entire pixel specified by pixel coordinates
void SoftwareRendererImp::fill_pixel(int x, int y, const Color &color) {

	// Task 2: Re-implement this function

	// check bounds
	if (x < 0 || x >= width) return;
	if (y < 0 || y >= height) return;

  // fill in corresponding sample buffer pixels
  for (int sx = x * sample_rate; sx < x * sample_rate + sample_rate; sx++) {
    for (int sy = y * sample_rate; sy < y * sample_rate + sample_rate; sy++) {
      fill_sample(sx, sy, color);
    }
  }
}

void SoftwareRendererImp::draw_svg( SVG& svg ) {

  if (sample_buffer == nullptr) {
    sample_buffer = new std::vector<unsigned char>(pow(sample_rate, 2) * width * height * 4);
  }
  std::fill(sample_buffer->begin(), sample_buffer->end(), 255);

  // set top level transformation
  transformation = canvas_to_screen;

  // canvas outline
  Vector2D a = transform(Vector2D(0, 0)); a.x--; a.y--;
  Vector2D b = transform(Vector2D(svg.width, 0)); b.x++; b.y--;
  Vector2D c = transform(Vector2D(0, svg.height)); c.x--; c.y++;
  Vector2D d = transform(Vector2D(svg.width, svg.height)); d.x++; d.y++;

  svg_bbox_top_left = Vector2D(a.x+1, a.y+1);
  svg_bbox_bottom_right = Vector2D(d.x-1, d.y-1);

  // draw all elements
  for (size_t i = 0; i < svg.elements.size(); ++i) {
    draw_element(svg.elements[i], transformation);
  }

  // draw canvas outline
  rasterize_line(a.x, a.y, b.x, b.y, Color::Black);
  rasterize_line(a.x, a.y, c.x, c.y, Color::Black);
  rasterize_line(d.x, d.y, b.x, b.y, Color::Black);
  rasterize_line(d.x, d.y, c.x, c.y, Color::Black);

  // resolve and send to pixel buffer
  resolve();
}

void SoftwareRendererImp::set_sample_rate( size_t sample_rate ) {

  // Task 2: 
  // You may want to modify this for supersampling support
  this->sample_rate = sample_rate;
  // realloc sample_buffer to correct size
  delete sample_buffer;
  sample_buffer = new std::vector<unsigned char>(pow(sample_rate, 2) * width * height * 4);
  std::fill(sample_buffer->begin(), sample_buffer->end(), 255);
}

void SoftwareRendererImp::set_pixel_buffer( unsigned char* pixel_buffer,
                                             size_t width, size_t height ) {

  // Task 2: 
  // You may want to modify this for supersampling support
  this->pixel_buffer = pixel_buffer;
  this->width = width;
  this->height = height;
  // realloc sample_buffer to correct size
  delete sample_buffer;
  sample_buffer = new std::vector<unsigned char>(pow(sample_rate, 2) * width * height * 4);
  std::fill(sample_buffer->begin(), sample_buffer->end(), 255);
}

void SoftwareRendererImp::draw_element( SVGElement* element, Matrix3x3 parent_transformation) {

	// Task 3 (part 1):
	// Modify this to implement the transformation stack
  transformation = parent_transformation * element->transform;
	switch (element->type) {
	case POINT:
		draw_point(static_cast<Point&>(*element));
		break;
	case LINE:
		draw_line(static_cast<Line&>(*element));
		break;
	case POLYLINE:
		draw_polyline(static_cast<Polyline&>(*element));
		break;
	case RECT:
		draw_rect(static_cast<Rect&>(*element));
		break;
	case POLYGON:
		draw_polygon(static_cast<Polygon&>(*element));
		break;
	case ELLIPSE:
		draw_ellipse(static_cast<Ellipse&>(*element));
		break;
	case IMAGE:
		draw_image(static_cast<Image&>(*element));
		break;
	case GROUP:
		draw_group(static_cast<Group&>(*element));
		break;
	default:
		break;
	}
  transformation = parent_transformation;
}


// Primitive Drawing //

void SoftwareRendererImp::draw_point( Point& point ) {
  Vector2D p = transform(point.position);
  rasterize_point( p.x, p.y, point.style.fillColor );

}

void SoftwareRendererImp::draw_line( Line& line ) { 

  Vector2D p0 = transform(line.from);
  Vector2D p1 = transform(line.to);
  rasterize_line( p0.x, p0.y, p1.x, p1.y, line.style.strokeColor );

}

void SoftwareRendererImp::draw_polyline( Polyline& polyline ) {

  Color c = polyline.style.strokeColor;

  if( c.a != 0 ) {
    int nPoints = polyline.points.size();
    for( int i = 0; i < nPoints - 1; i++ ) {
      Vector2D p0 = transform(polyline.points[(i+0) % nPoints]);
      Vector2D p1 = transform(polyline.points[(i+1) % nPoints]);
      rasterize_line( p0.x, p0.y, p1.x, p1.y, c );
    }
  }
}

void SoftwareRendererImp::draw_rect( Rect& rect ) {

  Color c;
  
  // draw as two triangles
  float x = rect.position.x;
  float y = rect.position.y;
  float w = rect.dimension.x;
  float h = rect.dimension.y;

  Vector2D p0 = transform(Vector2D(   x   ,   y   ));
  Vector2D p1 = transform(Vector2D( x + w ,   y   ));
  Vector2D p2 = transform(Vector2D(   x   , y + h ));
  Vector2D p3 = transform(Vector2D( x + w , y + h ));
  
  // draw fill
  c = rect.style.fillColor;
  if (c.a != 0 ) {
    rasterize_triangle( p0.x, p0.y, p1.x, p1.y, p2.x, p2.y, c );
    rasterize_triangle( p2.x, p2.y, p1.x, p1.y, p3.x, p3.y, c );
  }

  // draw outline
  c = rect.style.strokeColor;
  if( c.a != 0 ) {
    rasterize_line( p0.x, p0.y, p1.x, p1.y, c );
    rasterize_line( p1.x, p1.y, p3.x, p3.y, c );
    rasterize_line( p3.x, p3.y, p2.x, p2.y, c );
    rasterize_line( p2.x, p2.y, p0.x, p0.y, c );
  }

}

void SoftwareRendererImp::draw_polygon( Polygon& polygon ) {

  Color c;

  // draw fill
  c = polygon.style.fillColor;
  if( c.a != 0 ) {

    // triangulate
    vector<Vector2D> triangles;
    triangulate( polygon, triangles );

    // draw as triangles
    for (size_t i = 0; i < triangles.size(); i += 3) {
      Vector2D p0 = transform(triangles[i + 0]);
      Vector2D p1 = transform(triangles[i + 1]);
      Vector2D p2 = transform(triangles[i + 2]);
      rasterize_triangle( p0.x, p0.y, p1.x, p1.y, p2.x, p2.y, c );
    }
  }

  // draw outline
  c = polygon.style.strokeColor;
  if( c.a != 0 ) {
    int nPoints = polygon.points.size();
    for( int i = 0; i < nPoints; i++ ) {
      Vector2D p0 = transform(polygon.points[(i+0) % nPoints]);
      Vector2D p1 = transform(polygon.points[(i+1) % nPoints]);
      rasterize_line( p0.x, p0.y, p1.x, p1.y, c );
    }
  }
}

void SoftwareRendererImp::draw_ellipse( Ellipse& ellipse ) {

  // Advanced Task
  // Implement ellipse rasterization

}

void SoftwareRendererImp::draw_image( Image& image ) {

  // Advanced Task
  // Render image element with rotation

  Vector2D p0 = transform(image.position);
  Vector2D p1 = transform(image.position + image.dimension);

  rasterize_image( p0.x, p0.y, p1.x, p1.y, image.tex );
}

void SoftwareRendererImp::draw_group( Group& group ) {

  for ( size_t i = 0; i < group.elements.size(); ++i ) {
    draw_element(group.elements[i], transformation);
  }

}

// Rasterization //

// The input arguments in the rasterization functions 
// below are all defined in screen space coordinates

void SoftwareRendererImp::rasterize_point( float x, float y, Color color ) {

  // fill in the nearest pixel
  int sx = (int)floor(x);
  int sy = (int)floor(y);

  // check bounds
  if (sx < 0 || sx >= width) return;
  if (sy < 0 || sy >= height) return;

  fill_pixel(sx, sy,color);

}

void SoftwareRendererImp::rasterize_line( float x0, float y0,
                                          float x1, float y1,
                                          Color color) {

  // Task 0: 
  // Implement Bresenham's algorithm (delete the line below and implement your own)
  ref->rasterize_line_helper(x0, y0, x1, y1, width, height, color, this);
  // float m = (y1-y0)/(x1-x0);
  // if (abs(m) >= 1) {
  //   // Case where slope more than 1
  //   m = 1/m;
  //   if (y0 > y1) {
  //     std::swap(x0, x1);
  //     std::swap(y0, y1);
  //   }
  //   int x  = (int)floor(x0);
  //   float eps = x0 - (int)floor(x0);
  //   for (int y = (int)floor(y0); y <= y1; y++)  {
  //     fill_pixel(x, y, color);
  //     if (m >= 0) {
  //       // case with positive slope
  //       if (eps + m < 0.5) {
  //         eps = eps + m;
  //       } else {
  //           x++;
  //           eps = eps + m - 1;
  //       }
  //     } else {
  //       // case with negative slope
  //       if (eps + m > -0.5) {
  //         eps = eps + m;
  //       } else {
  //           x--;
  //           eps = eps + m + 1;
  //       }
  //     }
  //   }
  // } else {
  //   // Case where slope < 1
  //   if (x0 > x1) {
  //     std::swap(x0, x1);
  //     std::swap(y0, y1);
  //   }
  //   int y  = (int)floor(y0);
  //   float eps = y0 - (int)floor(y0);
  //   for (int x = (int)floor(x0); x <= x1; x++)  {
  //     fill_pixel(x, y, color);
  //     if (m >= 0) {
  //       // case with positive slope
  //       if (eps + m < 0.5) {
  //         eps = eps + m;
  //       } else {
  //           y++;
  //           eps = eps + m - 1;
  //       }
  //     } else {
  //       // case with negative slope
  //       if (eps + m > -0.5) {
  //         eps = eps + m;
  //       } else {
  //           y--;
  //           eps = eps + m + 1;
  //       }
  //     }
  //   }
  // }
 
  // Advanced Task
  // Drawing Smooth Lines with Line Width
}

void SoftwareRendererImp::rasterize_triangle( float x0, float y0,
                                              float x1, float y1,
                                              float x2, float y2,
                                              Color color ) {
  // Task 1: 
  // Implement triangle rasterization

  // Get bounding box values
  float minX = std::min(x0, x1);
  minX = std::min(minX, x2);
  float maxX = std::max(x0, x1);
  maxX = std::max(maxX, x2);
  float minY = std::min(y0, y1);
  minY = std::min(minY, y2);
  float maxY = std::max(y0, y1);
  maxY = std::max(maxY, y2);

  // Make sure orientation is counter-clockwise
  bool clockwise = (x1 - x0) * (y2 - y0) - (y1 - y0) * (x2 - x0) > 0;
  if (clockwise) {
    std::swap(x0, x1);
    std::swap(y0, y1);
  }

  // find all normal vectors
  float p01_norm[2] = {y1 - y0, -1 * (x1 - x0)};
  float p12_norm[2] = {y2 - y1, -1 * (x2 - x1)};
  float p20_norm[2] = {y0 - y2, -1 * (x0 - x2)};

  // Loop through all pixel centers within bounding box
  float interval = 1.0/sample_rate;
  for ( float x = floor(minX) + interval/2; x <= ceil(maxX) - interval/2; x += interval )  {
    for ( float y = floor(minY) + interval/2; y <= ceil(maxY) - interval/2; y += interval )  {
      float p01_v[2] = {x - x0, y - y0};
      float p12_v[2] = {x - x1, y - y1};
      float p20_v[2] = {x - x2, y - y2};

      bool inside_p01 = (p01_norm[0] * p01_v[0] + p01_norm[1] * p01_v[1]) >= 0;
      bool inside_p12 = (p12_norm[0] * p12_v[0] + p12_norm[1] * p12_v[1]) >= 0;
      bool inside_p20 = (p20_norm[0] * p20_v[0] + p20_norm[1] * p20_v[1]) >= 0;
      bool inside_tri = inside_p01 && inside_p12 && inside_p20;

      if (inside_tri) {
        int sx = (int)floor(x/interval);
        int sy = (int)floor(y/interval);
        fill_sample(sx, sy, color);
      }
    }
  }

  // Advanced Task
  // Implementing Triangle Edge Rules

}

void SoftwareRendererImp::rasterize_image( float x0, float y0,
                                           float x1, float y1,
                                           Texture& tex ) {
  // Task 4: 
  // Implement image rasterization

}

// resolve samples to pixel buffer
void SoftwareRendererImp::resolve( void ) {

  // Task 2: 
  // Implement supersampling
  // You may also need to modify other functions marked with "Task 2".
  for (int x = 0; x < width; x++) {
    for (int y = 0; y < height; y++) {
      // take average of sample buffer values
      uint16_t sample_r = 0;
      uint16_t sample_g = 0;
      uint16_t sample_b = 0;
      uint16_t sample_a = 0;
      for (int sx = x * sample_rate; sx < x * sample_rate + sample_rate; sx++) {
        for (int sy = y * sample_rate; sy < y * sample_rate + sample_rate; sy++) {
          sample_r += sample_buffer->at(4 * (sx + sy * width * sample_rate));
          sample_g +=  sample_buffer->at(4 * (sx + sy * width * sample_rate) + 1);
          sample_b += sample_buffer->at(4 * (sx + sy * width * sample_rate) + 2);
          sample_a += sample_buffer->at(4 * (sx + sy * width * sample_rate) + 3);
        }
      }
      sample_r = (uint8_t)(sample_r / (pow(sample_rate, 2)));
      sample_g = (uint8_t)(sample_g / (pow(sample_rate, 2)));
      sample_b = (uint8_t)(sample_b / (pow(sample_rate, 2)));
      sample_a = (uint8_t)(sample_a / (pow(sample_rate, 2)));
      pixel_buffer[4 * (x + y * width)] = sample_r;
      pixel_buffer[4 * (x + y * width) + 1] = sample_g;
      pixel_buffer[4 * (x + y * width) + 2] = sample_b;
      pixel_buffer[4 * (x + y * width) + 3] = sample_a;
    }
  }
}

Color SoftwareRendererImp::alpha_blending(Color pixel_color, Color color)
{
  // Task 5
  // Implement alpha compositing
  return pixel_color;
}


} // namespace CS248
