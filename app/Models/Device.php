<?php
namespace App\Models;
use Illuminate\Database\Eloquent\Model;
class Device extends Model {
    protected $fillable=['provider_id','device_uid','token_hash','last_seen_at'];
    protected $hidden=['token_hash'];
    protected function casts(): array { return ['last_seen_at'=>'datetime']; }
    public function provider(){return $this->belongsTo(Provider::class);}
}
